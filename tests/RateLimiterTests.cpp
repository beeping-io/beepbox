#include <catch2/catch_test_macros.hpp>
#include "beepbox/RateLimiter.h"

#include <thread>

// --- TokenBucket ---

TEST_CASE("Fresh bucket allows requests up to capacity", "[ratelimit]") {
  beepbox::TokenBucket bucket(5.0, 5.0 / 60.0);

  for (int i = 0; i < 5; ++i) {
    auto r = bucket.consume();
    REQUIRE(r.allowed);
    REQUIRE(r.remaining == 4 - i);
    REQUIRE(r.limit == 5);
  }

  // 6th request should be denied
  auto r = bucket.consume();
  REQUIRE_FALSE(r.allowed);
  REQUIRE(r.remaining == 0);
  REQUIRE(r.retryAfter >= 1);
}

TEST_CASE("Bucket refills after time passes", "[ratelimit]") {
  // 60 RPM = 1 per second refill
  beepbox::TokenBucket bucket(3.0, 1.0);

  // Drain all tokens
  for (int i = 0; i < 3; ++i) bucket.consume();
  auto denied = bucket.consume();
  REQUIRE_FALSE(denied.allowed);

  // Wait for refill (1 token per second)
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));

  auto r = bucket.consume();
  REQUIRE(r.allowed);
}

TEST_CASE("Denied response has Retry-After > 0", "[ratelimit]") {
  beepbox::TokenBucket bucket(1.0, 1.0 / 60.0);

  bucket.consume();  // Use the only token
  auto r = bucket.consume();
  REQUIRE_FALSE(r.allowed);
  REQUIRE(r.retryAfter >= 1);
}

// --- RateLimiter ---

TEST_CASE("Disabled limiter always allows", "[ratelimit]") {
  beepbox::RateLimiter limiter(0);
  REQUIRE_FALSE(limiter.enabled());

  auto r = limiter.check("bk_any");
  REQUIRE(r.allowed);
}

TEST_CASE("Per-key isolation — exhausting one key doesn't affect another", "[ratelimit]") {
  beepbox::RateLimiter limiter(3);

  // Exhaust key A
  for (int i = 0; i < 3; ++i) {
    auto r = limiter.check("bk_keyA");
    REQUIRE(r.allowed);
  }
  auto denied = limiter.check("bk_keyA");
  REQUIRE_FALSE(denied.allowed);

  // Key B should still have full capacity
  auto r = limiter.check("bk_keyB");
  REQUIRE(r.allowed);
  REQUIRE(r.remaining == 2);
}

TEST_CASE("Key count tracks unique keys", "[ratelimit]") {
  beepbox::RateLimiter limiter(10);
  REQUIRE(limiter.keyCount() == 0);

  limiter.check("bk_one");
  REQUIRE(limiter.keyCount() == 1);

  limiter.check("bk_two");
  REQUIRE(limiter.keyCount() == 2);

  // Same key again — no new entry
  limiter.check("bk_one");
  REQUIRE(limiter.keyCount() == 2);
}

TEST_CASE("Rate limit headers are correct", "[ratelimit]") {
  beepbox::RateLimiter limiter(5);

  auto r = limiter.check("bk_headers");
  REQUIRE(r.allowed);
  REQUIRE(r.limit == 5);
  REQUIRE(r.remaining == 4);
  REQUIRE(r.resetAt > 0);
}

TEST_CASE("429 result has correct fields", "[ratelimit]") {
  beepbox::RateLimiter limiter(1);

  limiter.check("bk_burst");  // Use the only token
  auto r = limiter.check("bk_burst");
  REQUIRE_FALSE(r.allowed);
  REQUIRE(r.limit == 1);
  REQUIRE(r.remaining == 0);
  REQUIRE(r.retryAfter >= 1);
  REQUIRE(r.resetAt > 0);
}

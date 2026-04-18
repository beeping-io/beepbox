#include "beepbox/RateLimiter.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace beepbox {

// --- TokenBucket ---

TokenBucket::TokenBucket(double capacity, double refillPerSecond)
    : capacity_(capacity),
      tokens_(capacity),
      refillPerSecond_(refillPerSecond),
      lastRefill_(std::chrono::steady_clock::now()) {}

void TokenBucket::refill() {
  auto now = std::chrono::steady_clock::now();
  double elapsed =
      std::chrono::duration<double>(now - lastRefill_).count();
  tokens_ = std::min(capacity_, tokens_ + elapsed * refillPerSecond_);
  lastRefill_ = now;
}

RateLimitResult TokenBucket::consume() {
  refill();

  auto now = std::chrono::steady_clock::now();
  int64_t resetAt = std::chrono::duration_cast<std::chrono::seconds>(
      now.time_since_epoch()).count();

  if (tokens_ >= 1.0) {
    tokens_ -= 1.0;
    // Reset time: when bucket would be full again
    double secondsToFull = (capacity_ - tokens_) / refillPerSecond_;
    resetAt += static_cast<int64_t>(std::ceil(secondsToFull));

    return {true,
            static_cast<int>(capacity_),
            static_cast<int>(tokens_),
            resetAt,
            0};
  }

  // Not enough tokens
  double secondsToOne = (1.0 - tokens_) / refillPerSecond_;
  int retryAfter = std::max(1, static_cast<int>(std::ceil(secondsToOne)));
  double secondsToFull = capacity_ / refillPerSecond_;
  resetAt += static_cast<int64_t>(std::ceil(secondsToFull));

  return {false,
          static_cast<int>(capacity_),
          0,
          resetAt,
          retryAfter};
}

// --- RateLimiter ---

RateLimiter::RateLimiter(int requestsPerMinute)
    : enabled_(requestsPerMinute > 0),
      rpm_(requestsPerMinute) {}

RateLimitResult RateLimiter::check(const std::string& key) {
  if (!enabled_) {
    return {true, 0, 0, 0, 0};
  }

  std::lock_guard<std::mutex> lock(mu_);
  auto it = buckets_.find(key);
  if (it == buckets_.end()) {
    double capacity = static_cast<double>(rpm_);
    double refillPerSecond = capacity / 60.0;
    auto [inserted, _] = buckets_.emplace(
        key, TokenBucket(capacity, refillPerSecond));
    it = inserted;
  }
  return it->second.consume();
}

size_t RateLimiter::keyCount() const {
  std::lock_guard<std::mutex> lock(mu_);
  return buckets_.size();
}

RateLimiter RateLimiter::fromEnv() {
  const char* env = std::getenv("BEEPBOX_RATE_LIMIT_RPM");
  if (!env) return RateLimiter(0);
  int rpm = std::atoi(env);
  return RateLimiter(rpm > 0 ? rpm : 0);
}

}  // namespace beepbox

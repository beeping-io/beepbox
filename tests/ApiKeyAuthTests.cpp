#include <catch2/catch_test_macros.hpp>
#include "beepbox/ApiKeyAuth.h"

#include <thread>

// Simple test KeyStore with fixed keys
class TestKeyStore : public beepbox::KeyStore {
 public:
  TestKeyStore(std::initializer_list<std::string> keys) : keys_(keys) {}
  bool validate(const std::string& key) override {
    lookups_++;
    return keys_.count(key) > 0;
  }
  int lookups() const { return lookups_; }

 private:
  std::unordered_set<std::string> keys_;
  int lookups_ = 0;
};

// --- Header parsing ---

TEST_CASE("Missing Authorization header returns Missing", "[auth]") {
  TestKeyStore store({"bk_test123"});
  beepbox::KeyCache cache;
  auto r = beepbox::checkAuth("", store, cache);
  REQUIRE(r.result == beepbox::AuthResult::Missing);
}

TEST_CASE("Non-Bearer header returns BadFormat", "[auth]") {
  TestKeyStore store({"bk_test123"});
  beepbox::KeyCache cache;
  auto r = beepbox::checkAuth("Basic dXNlcjpwYXNz", store, cache);
  REQUIRE(r.result == beepbox::AuthResult::BadFormat);
}

TEST_CASE("Bearer without bk_ prefix returns BadFormat", "[auth]") {
  TestKeyStore store({"bk_test123"});
  beepbox::KeyCache cache;
  auto r = beepbox::checkAuth("Bearer sk_notbeepbox", store, cache);
  REQUIRE(r.result == beepbox::AuthResult::BadFormat);
}

TEST_CASE("Valid key returns Ok", "[auth]") {
  TestKeyStore store({"bk_test123"});
  beepbox::KeyCache cache;
  auto r = beepbox::checkAuth("Bearer bk_test123", store, cache);
  REQUIRE(r.result == beepbox::AuthResult::Ok);
  REQUIRE(r.key == "bk_test123");
}

TEST_CASE("Invalid key returns InvalidKey", "[auth]") {
  TestKeyStore store({"bk_test123"});
  beepbox::KeyCache cache;
  auto r = beepbox::checkAuth("Bearer bk_wrong", store, cache);
  REQUIRE(r.result == beepbox::AuthResult::InvalidKey);
  REQUIRE(r.key == "bk_wrong");
}

// --- Cache behavior ---

TEST_CASE("Cache hit avoids store lookup", "[auth][cache]") {
  TestKeyStore store({"bk_cached"});
  beepbox::KeyCache cache;

  // First call: store lookup
  auto r1 = beepbox::checkAuth("Bearer bk_cached", store, cache);
  REQUIRE(r1.result == beepbox::AuthResult::Ok);
  REQUIRE(store.lookups() == 1);

  // Second call: cache hit, no store lookup
  auto r2 = beepbox::checkAuth("Bearer bk_cached", store, cache);
  REQUIRE(r2.result == beepbox::AuthResult::Ok);
  REQUIRE(store.lookups() == 1);  // Still 1
}

TEST_CASE("Cache stores invalid keys too", "[auth][cache]") {
  TestKeyStore store({"bk_good"});
  beepbox::KeyCache cache;

  auto r1 = beepbox::checkAuth("Bearer bk_bad", store, cache);
  REQUIRE(r1.result == beepbox::AuthResult::InvalidKey);
  REQUIRE(store.lookups() == 1);

  auto r2 = beepbox::checkAuth("Bearer bk_bad", store, cache);
  REQUIRE(r2.result == beepbox::AuthResult::InvalidKey);
  REQUIRE(store.lookups() == 1);  // Cached
}

TEST_CASE("Cache entry expires after TTL", "[auth][cache]") {
  TestKeyStore store({"bk_ttl"});
  // 1 second TTL for testing
  beepbox::KeyCache cache(1000, std::chrono::seconds(1));

  auto r1 = beepbox::checkAuth("Bearer bk_ttl", store, cache);
  REQUIRE(r1.result == beepbox::AuthResult::Ok);
  REQUIRE(store.lookups() == 1);

  // Wait for TTL to expire
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));

  auto r2 = beepbox::checkAuth("Bearer bk_ttl", store, cache);
  REQUIRE(r2.result == beepbox::AuthResult::Ok);
  REQUIRE(store.lookups() == 2);  // Re-validated
}

TEST_CASE("Cache size is tracked", "[auth][cache]") {
  TestKeyStore store({"bk_a", "bk_b", "bk_c"});
  beepbox::KeyCache cache;

  REQUIRE(cache.size() == 0);
  beepbox::checkAuth("Bearer bk_a", store, cache);
  REQUIRE(cache.size() == 1);
  beepbox::checkAuth("Bearer bk_b", store, cache);
  REQUIRE(cache.size() == 2);
  // Repeat bk_a — cache hit, no new entry
  beepbox::checkAuth("Bearer bk_a", store, cache);
  REQUIRE(cache.size() == 2);
}

// --- HTTP status codes ---

TEST_CASE("authStatusCode returns correct codes", "[auth]") {
  REQUIRE(beepbox::authStatusCode(beepbox::AuthResult::Missing) == 401);
  REQUIRE(beepbox::authStatusCode(beepbox::AuthResult::BadFormat) == 401);
  REQUIRE(beepbox::authStatusCode(beepbox::AuthResult::InvalidKey) == 403);
  REQUIRE(beepbox::authStatusCode(beepbox::AuthResult::Ok) == 200);
}

// --- Error bodies ---

TEST_CASE("authErrorBody returns JSON", "[auth]") {
  auto missing = beepbox::authErrorBody(beepbox::AuthResult::Missing);
  REQUIRE(missing.find("Missing API key") != std::string::npos);

  auto bad = beepbox::authErrorBody(beepbox::AuthResult::BadFormat);
  REQUIRE(bad.find("Invalid API key format") != std::string::npos);

  auto invalid = beepbox::authErrorBody(beepbox::AuthResult::InvalidKey);
  REQUIRE(invalid.find("Invalid API key") != std::string::npos);
}

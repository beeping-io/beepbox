#include <atomic>
#include <catch2/catch_test_macros.hpp>

#include "beepbox/HttpKeyStore.h"

using beepbox::HttpKeyStore;

TEST_CASE("HttpKeyStore: valid result returns true",
          "[auth][http-store]") {
  HttpKeyStore store([](const std::string& key) -> bool {
    return key == "bk_good";
  });
  REQUIRE(store.validate("bk_good"));
  REQUIRE(!store.validate("bk_other"));
}

TEST_CASE("HttpKeyStore: validator receives the raw key",
          "[auth][http-store]") {
  std::string captured;
  HttpKeyStore store([&captured](const std::string& key) -> bool {
    captured = key;
    return true;
  });
  store.validate("bk_abc123");
  REQUIRE(captured == "bk_abc123");
}

TEST_CASE("HttpKeyStore: forwards every call (no internal cache)",
          "[auth][http-store]") {
  std::atomic<int> calls{0};
  HttpKeyStore store([&calls](const std::string&) -> bool {
    calls.fetch_add(1);
    return true;
  });
  store.validate("bk_same");
  store.validate("bk_same");
  store.validate("bk_other");
  // KeyCache handles caching; HttpKeyStore does not.
  REQUIRE(calls.load() == 3);
}

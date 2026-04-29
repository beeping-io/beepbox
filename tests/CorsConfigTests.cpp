// SPDX-License-Identifier: Apache-2.0
// Copyright Beeping contributors

#include "beepbox/CorsConfig.h"

#include <catch2/catch_test_macros.hpp>

using beepbox::CorsConfig;

TEST_CASE("CorsConfig::parseAllowedOrigins", "[cors]") {
  SECTION("empty string yields no entries") {
    REQUIRE(CorsConfig::parseAllowedOrigins("").empty());
  }

  SECTION("single origin") {
    auto v = CorsConfig::parseAllowedOrigins("http://localhost:3000");
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "http://localhost:3000");
  }

  SECTION("multiple origins, trims whitespace, drops empties") {
    auto v = CorsConfig::parseAllowedOrigins(
        " http://localhost:3000 ,https://beeping.io,, https://www.beeping.io ");
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == "http://localhost:3000");
    REQUIRE(v[1] == "https://beeping.io");
    REQUIRE(v[2] == "https://www.beeping.io");
  }

  SECTION("preserves input order") {
    auto v = CorsConfig::parseAllowedOrigins("c,a,b");
    REQUIRE(v == std::vector<std::string>{"c", "a", "b"});
  }

  SECTION("a trailing comma does not introduce a phantom empty entry") {
    auto v = CorsConfig::parseAllowedOrigins("http://localhost:3000,");
    REQUIRE(v.size() == 1);
    REQUIRE(v[0] == "http://localhost:3000");
  }
}

TEST_CASE("CorsConfig::resolveAllowOrigin", "[cors]") {
  CorsConfig cfg({
      "http://localhost:3000",
      "https://beeping-platform-dev.web.app",
      "https://beeping.io",
  });

  SECTION("exact match returns the whitelisted entry") {
    auto out = cfg.resolveAllowOrigin("http://localhost:3000");
    REQUIRE(out.has_value());
    REQUIRE(*out == "http://localhost:3000");
  }

  SECTION("any of the whitelisted origins resolve") {
    REQUIRE(cfg.resolveAllowOrigin("https://beeping.io").has_value());
    REQUIRE(cfg.resolveAllowOrigin("https://beeping-platform-dev.web.app")
                .has_value());
  }

  SECTION("non-whitelisted origin returns nullopt") {
    REQUIRE_FALSE(cfg.resolveAllowOrigin("https://evil.com").has_value());
  }

  SECTION("empty origin returns nullopt (no Origin header → not a CORS req)") {
    REQUIRE_FALSE(cfg.resolveAllowOrigin("").has_value());
  }

  SECTION("scheme/host/port are matched verbatim — no fuzzy matching") {
    // http vs https
    REQUIRE_FALSE(cfg.resolveAllowOrigin("https://localhost:3000").has_value());
    // different port
    REQUIRE_FALSE(cfg.resolveAllowOrigin("http://localhost:3001").has_value());
    // trailing slash mismatch
    REQUIRE_FALSE(cfg.resolveAllowOrigin("http://localhost:3000/").has_value());
    // case-sensitive (RFC 6454 origin comparison is case-sensitive on host)
    REQUIRE_FALSE(cfg.resolveAllowOrigin("HTTP://localhost:3000").has_value());
  }
}

TEST_CASE("CorsConfig::enabled", "[cors]") {
  SECTION("default-constructed is disabled") {
    REQUIRE_FALSE(CorsConfig{}.enabled());
  }

  SECTION("empty whitelist is disabled") {
    REQUIRE_FALSE(CorsConfig{{}}.enabled());
  }

  SECTION("non-empty whitelist is enabled") {
    REQUIRE(CorsConfig{{"http://localhost:3000"}}.enabled());
  }
}

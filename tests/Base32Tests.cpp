#include <catch2/catch_test_macros.hpp>
#include "beepbox/Base32.h"

using namespace beepbox;

TEST_CASE("Base32: toBase32 basic values", "[base32]") {
  CHECK(toBase32(0) == "0");
  CHECK(toBase32(1) == "1");
  CHECK(toBase32(10) == "a");
  CHECK(toBase32(31) == "v");
  CHECK(toBase32(32) == "10");
  CHECK(toBase32(1023) == "vv");
}

TEST_CASE("Base32: fromBase32 basic values", "[base32]") {
  CHECK(fromBase32("0") == 0);
  CHECK(fromBase32("1") == 1);
  CHECK(fromBase32("a") == 10);
  CHECK(fromBase32("v") == 31);
  CHECK(fromBase32("10") == 32);
  CHECK(fromBase32("vv") == 1023);
}

TEST_CASE("Base32: round-trip toBase32 <-> fromBase32", "[base32]") {
  for (int i = 0; i < 10000; i += 37) {
    CHECK(fromBase32(toBase32(i)) == i);
  }
}

TEST_CASE("Base32: isBase32Char", "[base32]") {
  for (char c = '0'; c <= '9'; ++c) CHECK(isBase32Char(c));
  for (char c = 'a'; c <= 'v'; ++c) CHECK(isBase32Char(c));
  CHECK_FALSE(isBase32Char('w'));
  CHECK_FALSE(isBase32Char('z'));
  CHECK_FALSE(isBase32Char('A'));
  CHECK_FALSE(isBase32Char(' '));
}

TEST_CASE("Base32: isValidKey", "[base32]") {
  CHECK(isValidKey("0abc1"));
  CHECK(isValidKey("vvvvv"));
  CHECK(isValidKey("00000"));
  CHECK_FALSE(isValidKey(""));
  CHECK_FALSE(isValidKey("abc"));       // too short
  CHECK_FALSE(isValidKey("abcdef"));    // too long
  CHECK_FALSE(isValidKey("0abcw"));     // 'w' invalid
  CHECK_FALSE(isValidKey("0ABC1"));     // uppercase invalid
}

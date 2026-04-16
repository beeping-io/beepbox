#include <catch2/catch_test_macros.hpp>
#include "beepbox/Params.h"

using namespace beepbox;

TEST_CASE("Params: valid default params", "[params]") {
  Params p;
  p.key = "0abc1";
  auto r = validate(p);
  CHECK(r.ok);
  CHECK(r.errors.empty());
}

TEST_CASE("Params: invalid key length", "[params]") {
  Params p;
  p.key = "abc";
  auto r = validate(p);
  CHECK_FALSE(r.ok);
  CHECK(r.errors.size() == 1);
}

TEST_CASE("Params: invalid key character", "[params]") {
  Params p;
  p.key = "0abcw";
  auto r = validate(p);
  CHECK_FALSE(r.ok);
}

TEST_CASE("Params: sample rate out of range", "[params]") {
  Params p;
  p.key = "0abc1";
  p.sampleRate = 8000.0f;
  auto r = validate(p);
  CHECK_FALSE(r.ok);
}

TEST_CASE("Params: sample rate valid range", "[params]") {
  for (float sr : {22050.0f, 44100.0f, 48000.0f, 96000.0f}) {
    Params p;
    p.key = "0abc1";
    p.sampleRate = sr;
    auto r = validate(p);
    CHECK(r.ok);
  }
}

TEST_CASE("Params: duration too short", "[params]") {
  Params p;
  p.key = "0abc1";
  p.duration = 1.0f;
  auto r = validate(p);
  CHECK_FALSE(r.ok);
}

TEST_CASE("Params: duration with start offset", "[params]") {
  Params p;
  p.key = "0abc1";
  p.startTime = 1.0f;
  p.duration = 2.5f; // needs start + 2.3 = 3.3
  auto r = validate(p);
  CHECK_FALSE(r.ok);
}

TEST_CASE("Params: interval too short", "[params]") {
  Params p;
  p.key = "0abc1";
  p.interval = 1.0f;
  auto r = validate(p);
  CHECK_FALSE(r.ok);
}

TEST_CASE("Params: volume warnings", "[params]") {
  Params p;
  p.key = "0abc1";
  p.volumeBeepsdB = -100.0f;
  auto r = validate(p);
  CHECK(r.ok); // warnings, not errors
  CHECK(r.warnings.size() >= 1);
}

TEST_CASE("Params: parseMode", "[params]") {
  Mode m;
  CHECK(parseMode("audible", m));   CHECK(m == Mode::Audible);
  CHECK(parseMode("inaudible", m)); CHECK(m == Mode::Inaudible);
  CHECK(parseMode("all", m));       CHECK(m == Mode::All);
  CHECK_FALSE(parseMode("hidden", m));
  CHECK_FALSE(parseMode("custom", m));
  CHECK_FALSE(parseMode("", m));
}

TEST_CASE("Params: parseMixMode", "[params]") {
  MixMode m;
  CHECK(parseMixMode("default", m)); CHECK(m == MixMode::Default);
  CHECK(parseMixMode("global", m));  CHECK(m == MixMode::Global);
  CHECK(parseMixMode("dynamic", m)); CHECK(m == MixMode::Dynamic);
  CHECK_FALSE(parseMixMode("xyz", m));
}

TEST_CASE("Params: toCoreMode mapping", "[params]") {
  CHECK(toCoreMode(Mode::Audible) == 2);   // BEEPING_MODE_AUDIBLE
  CHECK(toCoreMode(Mode::Inaudible) == 3); // BEEPING_MODE_INAUDIBLE
  CHECK(toCoreMode(Mode::All) == 5);       // BEEPING_MODE_ALL
}

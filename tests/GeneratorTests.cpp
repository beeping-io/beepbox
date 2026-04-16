#include <catch2/catch_test_macros.hpp>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"
#include <cmath>

using namespace beepbox;

static Params makeDefaultParams() {
  Params p;
  p.key = "0abc1";
  p.mode = Mode::Inaudible;
  p.sampleRate = 44100.0f;
  p.duration = 3.0f;
  p.startTime = 0.0f;
  p.interval = 2.3f;
  p.volumeBeepsdB = -3.0f;
  return p;
}

TEST_CASE("Generator: generateBeeps produces non-empty output", "[generator]") {
  auto result = generateBeeps(makeDefaultParams());
  CHECK(result.beepsGenerated >= 1);
  CHECK(!result.samples.empty());
  CHECK(result.sampleRate == 44100.0f);
}

TEST_CASE("Generator: output samples are in valid range", "[generator]") {
  auto result = generateBeeps(makeDefaultParams());
  float maxAbs = 0.0f;
  for (float s : result.samples) {
    float a = std::fabs(s);
    if (a > maxAbs) maxAbs = a;
  }
  CHECK(maxAbs > 0.0f);    // not silence
  CHECK(maxAbs < 2.0f);    // reasonable range
}

TEST_CASE("Generator: all 3 modes produce output", "[generator]") {
  for (Mode m : {Mode::Audible, Mode::Inaudible, Mode::All}) {
    auto p = makeDefaultParams();
    p.mode = m;
    auto result = generateBeeps(p);
    CHECK(result.beepsGenerated >= 1);
    CHECK(!result.samples.empty());
  }
}

TEST_CASE("Generator: generateBeepsForHost pads to host length", "[generator]") {
  auto p = makeDefaultParams();
  int hostSamples = 44100 * 5; // 5 seconds
  auto result = generateBeepsForHost(p, hostSamples, 44100.0f);
  CHECK(static_cast<int>(result.samples.size()) == hostSamples);
}

TEST_CASE("Generator: different keys produce different output", "[generator]") {
  auto p1 = makeDefaultParams();
  p1.key = "0abc1";
  auto r1 = generateBeeps(p1);

  auto p2 = makeDefaultParams();
  p2.key = "vvvvv";
  auto r2 = generateBeeps(p2);

  // At least some samples should differ
  REQUIRE(r1.samples.size() == r2.samples.size());
  bool allSame = true;
  for (size_t i = 0; i < r1.samples.size(); ++i) {
    if (r1.samples[i] != r2.samples[i]) { allSame = false; break; }
  }
  CHECK_FALSE(allSame);
}

TEST_CASE("Generator: 48kHz works", "[generator]") {
  auto p = makeDefaultParams();
  p.sampleRate = 48000.0f;
  auto result = generateBeeps(p);
  CHECK(result.beepsGenerated >= 1);
  CHECK(result.sampleRate == 48000.0f);
}

#include <catch2/catch_test_macros.hpp>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"
#include <BeepingCoreLib_api.h>
#include <cstring>
#include <string>

using namespace beepbox;

static constexpr int kDecodeChunk = 1024;

static std::string decodeFromSamples(const float* samples, int totalSamples,
                                     int coreMode, float sampleRate) {
  void* core = BEEPING_Create();
  BEEPING_Configure(coreMode, sampleRate, kDecodeChunk, core);

  int status = -1;

  // Feed all audio
  for (int offset = 0; offset < totalSamples; offset += kDecodeChunk) {
    int chunkSize = std::min(kDecodeChunk, totalSamples - offset);
    status = BEEPING_DecodeAudioBuffer(
        const_cast<float*>(samples + offset), chunkSize, core);
    if (status == -3) break; // -3 = complete word decoded
  }

  // Flush with silence to push remaining frames through decoder
  if (status != -3) {
    std::vector<float> silence(kDecodeChunk, 0.0f);
    for (int flush = 0; flush < 200; ++flush) {
      status = BEEPING_DecodeAudioBuffer(silence.data(), kDecodeChunk, core);
      if (status == -3) break;
    }
  }

  std::string decoded;
  if (status == -3) {
    char buf[256] = {};
    int rc = BEEPING_GetDecodedData(buf, core);
    if (rc > 0) {
      decoded = std::string(buf, rc);
    }
  }

  BEEPING_Destroy(core);
  return decoded;
}

TEST_CASE("RoundTrip: direct encode-decode Audible 44100", "[roundtrip]") {
  // Direct encode→decode bypassing Generator scheduling (same pattern as beeping-core tests)
  void* encoder = BEEPING_Create();
  BEEPING_Configure(BEEPING_MODE_AUDIBLE, 44100.0f, 128, encoder);

  const char* payload = "0abc10000";
  BEEPING_EncodeDataToAudioBuffer(payload, 9, 0, 0, 0, encoder);

  std::vector<float> audio;
  float buf[128];
  int got;
  do {
    got = BEEPING_GetEncodedAudioBuffer(buf, encoder);
    for (int i = 0; i < got; ++i) audio.push_back(buf[i]);
  } while (got > 0);
  BEEPING_Destroy(encoder);

  REQUIRE(!audio.empty());

  std::string decoded = decodeFromSamples(
      audio.data(), static_cast<int>(audio.size()),
      BEEPING_MODE_AUDIBLE, 44100.0f);

  REQUIRE(decoded.size() >= 5);
  CHECK(decoded.substr(0, 5) == "0abc1");
}

TEST_CASE("RoundTrip: direct encode-decode Inaudible 44100", "[roundtrip]") {
  void* encoder = BEEPING_Create();
  BEEPING_Configure(BEEPING_MODE_INAUDIBLE, 44100.0f, 128, encoder);

  const char* payload = "0abc10000";
  BEEPING_EncodeDataToAudioBuffer(payload, 9, 0, 0, 0, encoder);

  std::vector<float> audio;
  float buf[128];
  int got;
  do {
    got = BEEPING_GetEncodedAudioBuffer(buf, encoder);
    for (int i = 0; i < got; ++i) audio.push_back(buf[i]);
  } while (got > 0);
  BEEPING_Destroy(encoder);

  REQUIRE(!audio.empty());

  std::string decoded = decodeFromSamples(
      audio.data(), static_cast<int>(audio.size()),
      BEEPING_MODE_INAUDIBLE, 44100.0f);

  REQUIRE(decoded.size() >= 5);
  CHECK(decoded.substr(0, 5) == "0abc1");
}

TEST_CASE("RoundTrip: Generator output contains decodable audio — Audible 44100",
          "[roundtrip]") {
  Params p;
  p.key = "0abc1";
  p.mode = Mode::Audible;
  p.sampleRate = 44100.0f;
  p.duration = 4.0f;
  p.interval = 2.3f;

  auto result = generateBeeps(p);
  REQUIRE(result.beepsGenerated >= 1);

  // The Generator output includes silence + beep + silence.
  // Decode should find the payload somewhere in there.
  std::string decoded = decodeFromSamples(
      result.samples.data(), static_cast<int>(result.samples.size()),
      toCoreMode(Mode::Audible), p.sampleRate);

  // Payload is key + base32 timestamp. Key starts with "0abc1".
  CHECK(decoded.size() >= 5);
  if (decoded.size() >= 5) {
    CHECK(decoded.substr(0, 5) == "0abc1");
  }
}

TEST_CASE("RoundTrip: decode with ALL mode detects Inaudible", "[roundtrip]") {
  void* encoder = BEEPING_Create();
  BEEPING_Configure(BEEPING_MODE_INAUDIBLE, 44100.0f, 128, encoder);

  const char* payload = "123450000";
  BEEPING_EncodeDataToAudioBuffer(payload, 9, 0, 0, 0, encoder);

  std::vector<float> audio;
  float buf[128];
  int got;
  do {
    got = BEEPING_GetEncodedAudioBuffer(buf, encoder);
    for (int i = 0; i < got; ++i) audio.push_back(buf[i]);
  } while (got > 0);
  BEEPING_Destroy(encoder);

  std::string decoded = decodeFromSamples(
      audio.data(), static_cast<int>(audio.size()),
      BEEPING_MODE_ALL, 44100.0f);

  REQUIRE(decoded.size() >= 5);
  CHECK(decoded.substr(0, 5) == "12345");
}

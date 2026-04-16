#include "beepbox/Generator.h"
#include "beepbox/Base32.h"
#include "beepbox/Scheduler.h"
#include <BeepingCoreLib_api.h>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace beepbox {

static constexpr int kBufferSize = 128;
// Duration of a single token in seconds (from BeepingConfig default).
static constexpr double kDurToken = 0.104489796;

static void encodeBeepsIntoBuffer(
    void* core, const Params& p,
    std::vector<float>& outSamples,
    float effectiveSampleRate, float effectiveDuration,
    int beepsToGenerate, const std::vector<double>& schedule) {

  double durToken = kDurToken;
  double currentTime = 0.0;
  double nextMarkTime = p.startTime;
  int beepsGenerated = 0;
  float defBeepLevel = std::pow(10.0f, std::clamp(p.volumeBeepsdB, -60.0f, 12.0f) / 20.0f);

  float silenceBuf[kBufferSize] = {};
  float audioBuf[kBufferSize] = {};

  while (currentTime < effectiveDuration) {
    if (beepsGenerated < beepsToGenerate &&
        currentTime >= (nextMarkTime - durToken * 20.0)) {
      int timestampSec = static_cast<int>(nextMarkTime + 0.5f);
      std::string ts = toBase32(timestampSec);
      while (ts.size() < 4) ts = "0" + ts;

      std::string payload = p.key + ts;
      int sizeAudioBuffer = BEEPING_EncodeDataToAudioBuffer(
          payload.c_str(), static_cast<int>(payload.size()), 0, 0, 0, core);
      (void)sizeAudioBuffer;

      int samplesRetrieved = 0;
      do {
        std::memset(audioBuf, 0, kBufferSize * sizeof(float));
        samplesRetrieved = BEEPING_GetEncodedAudioBuffer(audioBuf, core);
        for (int i = 0; i < samplesRetrieved; ++i) {
          outSamples.push_back(defBeepLevel * audioBuf[i]);
        }
        currentTime += static_cast<double>(samplesRetrieved) / effectiveSampleRate;
      } while (samplesRetrieved > 0);

      BEEPING_ResetEncodedAudioBuffer(core);
      beepsGenerated++;
      nextMarkTime += p.interval;
    } else {
      for (int i = 0; i < kBufferSize; ++i) {
        outSamples.push_back(0.0f);
      }
      currentTime += static_cast<double>(kBufferSize) / effectiveSampleRate;
    }
  }
}

GenerateResult generateBeeps(const Params& p) {
  GenerateResult result;
  result.sampleRate = p.sampleRate;

  int beepCount = computeBeepCount(p.duration, p.startTime, p.interval);
  if (beepCount <= 0) return result;

  auto schedule = computeBeepSchedule(p.duration, p.startTime, p.interval);

  void* core = BEEPING_Create();
  BEEPING_Configure(toCoreMode(p.mode), p.sampleRate, kBufferSize, core);

  encodeBeepsIntoBuffer(core, p, result.samples, p.sampleRate, p.duration, beepCount, schedule);
  result.beepsGenerated = beepCount;

  BEEPING_Destroy(core);
  return result;
}

GenerateResult generateBeepsForHost(const Params& p, int hostSamples, float hostSampleRate) {
  GenerateResult result;
  result.sampleRate = hostSampleRate;

  float hostDuration = static_cast<float>(hostSamples) / hostSampleRate;
  int beepCount = computeBeepCount(hostDuration, p.startTime, p.interval);
  if (beepCount <= 0) return result;

  auto schedule = computeBeepSchedule(hostDuration, p.startTime, p.interval);

  void* core = BEEPING_Create();
  BEEPING_Configure(toCoreMode(p.mode), hostSampleRate, kBufferSize, core);

  encodeBeepsIntoBuffer(core, p, result.samples, hostSampleRate, hostDuration, beepCount, schedule);
  result.beepsGenerated = beepCount;

  // Pad or trim to match host length
  result.samples.resize(hostSamples, 0.0f);

  BEEPING_Destroy(core);
  return result;
}

} // namespace beepbox

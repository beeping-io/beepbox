#include "beepbox/Generator.h"

#include <BeepingCoreLib_api.h>

#include <cstdint>

namespace beepbox {

namespace {

// Internal exchange buffer size for BEEPING_Configure. Not exposed to
// callers — beeping-core handles internal segmentation. Matches the value
// used in the pre-refactor implementation for parity.
constexpr int kConfigBufferSize = 128;

GenerateResult encodeViaCoreSchedule(const Params& p,
                                     float effectiveSampleRate,
                                     float effectiveDuration) {
  GenerateResult result;
  result.sampleRate = effectiveSampleRate;

  void* core = BEEPING_Create();
  if (!core) return result;

  BEEPING_Configure(toCoreMode(p.mode), effectiveSampleRate,
                    kConfigBufferSize, core);

  int32_t required =
      BEEPING_GetScheduleBufferSize(effectiveDuration, core);
  if (required <= 0) {
    BEEPING_Destroy(core);
    return result;
  }
  result.samples.assign(static_cast<std::size_t>(required), 0.0f);

  int32_t samplesWritten = 0;
  int32_t rc = BEEPING_EncodeWithSchedule(
      p.key.c_str(), static_cast<int32_t>(p.key.size()),
      /*type=*/0, /*melody=*/nullptr, /*melodySize=*/0,
      effectiveDuration, p.startTime, p.interval,
      p.volumeBeepsdB,
      result.samples.data(), static_cast<int32_t>(result.samples.size()),
      &samplesWritten, core);

  if (rc != 0) {
    result.samples.clear();
    result.beepsGenerated = 0;
    BEEPING_Destroy(core);
    return result;
  }
  if (samplesWritten >= 0 &&
      static_cast<std::size_t>(samplesWritten) < result.samples.size()) {
    result.samples.resize(static_cast<std::size_t>(samplesWritten));
  }

  int32_t beepCount = 0;
  BEEPING_ComputeBeepSchedule(effectiveDuration, p.startTime, p.interval,
                              nullptr, 0, &beepCount);
  result.beepsGenerated = beepCount;

  BEEPING_Destroy(core);
  return result;
}

}  // namespace

GenerateResult generateBeeps(const Params& p) {
  return encodeViaCoreSchedule(p, p.sampleRate, p.duration);
}

GenerateResult generateBeepsForHost(const Params& p, int hostSamples,
                                    float hostSampleRate) {
  float hostDuration = static_cast<float>(hostSamples) / hostSampleRate;
  auto result = encodeViaCoreSchedule(p, hostSampleRate, hostDuration);
  if (!result.samples.empty()) {
    result.samples.resize(static_cast<std::size_t>(hostSamples), 0.0f);
  }
  return result;
}

}  // namespace beepbox

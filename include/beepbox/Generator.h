#ifndef BEEPBOX_GENERATOR_H
#define BEEPBOX_GENERATOR_H

#include "Params.h"
#include <vector>
#include <string>

namespace beepbox {

/// Result of beep generation: a buffer of PCM float samples.
struct GenerateResult {
  std::vector<float> samples;
  float sampleRate = 0.0f;
  int beepsGenerated = 0;
};

/// Generate beeps-only audio buffer (no mixing with host audio).
/// Returns PCM float samples at the requested sample rate.
GenerateResult generateBeeps(const Params& p);

/// Generate beeps buffer aligned to a host audio length.
/// Returns the beeps-only buffer (same length as hostSamples).
/// Caller is responsible for mixing.
GenerateResult generateBeepsForHost(const Params& p, int hostSamples, float hostSampleRate);

} // namespace beepbox

#endif // BEEPBOX_GENERATOR_H

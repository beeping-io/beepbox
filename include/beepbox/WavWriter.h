#ifndef BEEPBOX_WAV_WRITER_H
#define BEEPBOX_WAV_WRITER_H

#include <cstdint>
#include <vector>
#include <string>

namespace beepbox {

/// Generate a WAV file in memory from PCM float samples.
/// Returns the complete WAV file as a byte vector (16-bit PCM, mono).
std::vector<char> toWav(const float* samples, int numSamples, int sampleRate);

} // namespace beepbox

#endif // BEEPBOX_WAV_WRITER_H

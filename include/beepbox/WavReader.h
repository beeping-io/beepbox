#ifndef BEEPBOX_WAV_READER_H
#define BEEPBOX_WAV_READER_H

#include <vector>
#include <string>

namespace beepbox {

struct WavData {
  std::vector<float> samples;
  int sampleRate = 0;
  int channels = 0;
  bool valid = false;
  std::string error;
};

/// Parse a WAV file from memory (16-bit PCM, mono or stereo → downmix to mono).
WavData fromWav(const char* data, size_t size);

} // namespace beepbox

#endif // BEEPBOX_WAV_READER_H

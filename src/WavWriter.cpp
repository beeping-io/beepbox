#include "beepbox/WavWriter.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace beepbox {

static void writeU32LE(std::vector<char>& out, uint32_t val) {
  out.push_back(static_cast<char>(val & 0xFF));
  out.push_back(static_cast<char>((val >> 8) & 0xFF));
  out.push_back(static_cast<char>((val >> 16) & 0xFF));
  out.push_back(static_cast<char>((val >> 24) & 0xFF));
}

static void writeU16LE(std::vector<char>& out, uint16_t val) {
  out.push_back(static_cast<char>(val & 0xFF));
  out.push_back(static_cast<char>((val >> 8) & 0xFF));
}

std::vector<char> toWav(const float* samples, int numSamples, int sampleRate) {
  constexpr int kBitsPerSample = 16;
  constexpr int kNumChannels = 1;
  int byteRate = sampleRate * kNumChannels * kBitsPerSample / 8;
  int blockAlign = kNumChannels * kBitsPerSample / 8;
  int dataSize = numSamples * blockAlign;

  std::vector<char> wav;
  wav.reserve(44 + dataSize);

  // RIFF header
  wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
  writeU32LE(wav, 36 + dataSize);
  wav.insert(wav.end(), {'W', 'A', 'V', 'E'});

  // fmt subchunk
  wav.insert(wav.end(), {'f', 'm', 't', ' '});
  writeU32LE(wav, 16);                           // subchunk size
  writeU16LE(wav, 1);                            // PCM format
  writeU16LE(wav, kNumChannels);
  writeU32LE(wav, static_cast<uint32_t>(sampleRate));
  writeU32LE(wav, static_cast<uint32_t>(byteRate));
  writeU16LE(wav, static_cast<uint16_t>(blockAlign));
  writeU16LE(wav, kBitsPerSample);

  // data subchunk
  wav.insert(wav.end(), {'d', 'a', 't', 'a'});
  writeU32LE(wav, static_cast<uint32_t>(dataSize));

  // Convert float samples to 16-bit PCM
  for (int i = 0; i < numSamples; ++i) {
    float clamped = std::clamp(samples[i], -1.0f, 1.0f);
    int16_t pcm = static_cast<int16_t>(clamped * 32767.0f);
    wav.push_back(static_cast<char>(pcm & 0xFF));
    wav.push_back(static_cast<char>((pcm >> 8) & 0xFF));
  }

  return wav;
}

} // namespace beepbox

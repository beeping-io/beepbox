#include "beepbox/WavReader.h"
#include <cstring>

namespace beepbox {

static uint16_t readU16LE(const char* p) {
  auto u = reinterpret_cast<const uint8_t*>(p);
  return static_cast<uint16_t>(u[0] | (u[1] << 8));
}

static uint32_t readU32LE(const char* p) {
  auto u = reinterpret_cast<const uint8_t*>(p);
  return u[0] | (u[1] << 8) | (u[2] << 16) | (u[3] << 24);
}

WavData fromWav(const char* data, size_t size) {
  WavData result;

  if (size < 44) {
    result.error = "Data too small for WAV header";
    return result;
  }

  // RIFF header
  if (std::memcmp(data, "RIFF", 4) != 0) {
    result.error = "Not a RIFF file";
    return result;
  }
  if (std::memcmp(data + 8, "WAVE", 4) != 0) {
    result.error = "Not a WAVE file";
    return result;
  }

  // Find fmt and data chunks
  size_t pos = 12;
  int bitsPerSample = 0;
  int blockAlign = 0;
  int audioFormat = 0;
  size_t dataOffset = 0;
  uint32_t dataSize = 0;

  while (pos + 8 <= size) {
    uint32_t chunkSize = readU32LE(data + pos + 4);

    if (std::memcmp(data + pos, "fmt ", 4) == 0) {
      if (pos + 8 + chunkSize > size || chunkSize < 16) {
        result.error = "Invalid fmt chunk";
        return result;
      }
      const char* fmt = data + pos + 8;
      audioFormat = readU16LE(fmt);
      result.channels = readU16LE(fmt + 2);
      result.sampleRate = static_cast<int>(readU32LE(fmt + 4));
      blockAlign = readU16LE(fmt + 12);
      bitsPerSample = readU16LE(fmt + 14);
    } else if (std::memcmp(data + pos, "data", 4) == 0) {
      dataOffset = pos + 8;
      dataSize = chunkSize;
    }

    pos += 8 + chunkSize;
    if (chunkSize % 2 != 0) pos++; // padding byte
  }

  if (audioFormat != 1) {
    result.error = "Only PCM format supported (audioFormat=" +
                   std::to_string(audioFormat) + ")";
    return result;
  }
  if (bitsPerSample != 16) {
    result.error = "Only 16-bit PCM supported (got " +
                   std::to_string(bitsPerSample) + " bits)";
    return result;
  }
  if (dataOffset == 0 || dataSize == 0) {
    result.error = "No data chunk found";
    return result;
  }
  if (dataOffset + dataSize > size) {
    // Truncated — use what we have
    dataSize = static_cast<uint32_t>(size - dataOffset);
  }
  if (result.channels < 1 || result.channels > 2) {
    result.error = "Only mono or stereo supported (got " +
                   std::to_string(result.channels) + " channels)";
    return result;
  }

  // Parse 16-bit PCM samples → float mono
  int totalFrames = static_cast<int>(dataSize) / blockAlign;
  result.samples.resize(totalFrames);

  const char* p = data + dataOffset;
  for (int i = 0; i < totalFrames; ++i) {
    if (result.channels == 1) {
      int16_t s;
      std::memcpy(&s, p + i * 2, 2);
      result.samples[i] = static_cast<float>(s) / 32768.0f;
    } else {
      // Stereo → mono downmix
      int16_t left, right;
      std::memcpy(&left, p + i * 4, 2);
      std::memcpy(&right, p + i * 4 + 2, 2);
      result.samples[i] = (static_cast<float>(left) + static_cast<float>(right)) /
                           (2.0f * 32768.0f);
    }
  }

  result.valid = true;
  return result;
}

} // namespace beepbox

#include <catch2/catch_test_macros.hpp>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#ifndef BEEPBOX_GOLDEN_HASH_PATH
#error "BEEPBOX_GOLDEN_HASH_PATH must be defined by the build system"
#endif

namespace {

// FNV-1a 64-bit — deterministic across platforms, sufficient for
// binary-equivalence regression detection of the encode output buffer.
std::string fnv1a64Hex(const float* data, std::size_t count) {
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(data);
  std::size_t nBytes = count * sizeof(float);
  std::uint64_t h = 14695981039346656037ULL;
  for (std::size_t i = 0; i < nBytes; ++i) {
    h ^= static_cast<std::uint64_t>(bytes[i]);
    h *= 1099511628211ULL;
  }
  std::ostringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(16) << h;
  return ss.str();
}

std::string readFile(const std::string& path) {
  std::ifstream in(path);
  if (!in.is_open()) return {};
  std::string s((std::istreambuf_iterator<char>(in)),
                std::istreambuf_iterator<char>());
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' ||
                        s.back() == ' ' || s.back() == '\t')) {
    s.pop_back();
  }
  return s;
}

}  // namespace

TEST_CASE("Encode golden: deterministic buffer hash for fixed params",
          "[golden][encode]") {
  beepbox::Params p;
  p.key            = "abcde";
  p.mode           = beepbox::Mode::Inaudible;
  p.sampleRate     = 44100.0f;
  p.duration       = 10.0f;
  p.startTime      = 0.0f;
  p.interval       = 2.3f;
  p.volumeBeepsdB  = -3.0f;

  auto result = beepbox::generateBeeps(p);
  REQUIRE(result.beepsGenerated > 0);
  REQUIRE(!result.samples.empty());

  // Hash only the canonical sample window — floor(duration * sampleRate) —
  // so the test is invariant to harmless trailing-silence padding from
  // buffer-aligned emit loops.
  const std::size_t kCanonical =
      static_cast<std::size_t>(p.duration * p.sampleRate);
  REQUIRE(result.samples.size() >= kCanonical);
  const std::string computed =
      fnv1a64Hex(result.samples.data(), kCanonical);
  const std::string expected = readFile(BEEPBOX_GOLDEN_HASH_PATH);

  INFO("Computed FNV-1a 64-bit hash: " << computed);
  INFO("Expected (from " << BEEPBOX_GOLDEN_HASH_PATH << "): "
       << (expected.empty() ? std::string("<missing>") : expected));
  INFO("Sample count: " << result.samples.size()
       << " | Beeps: " << result.beepsGenerated);

  if (expected.empty() || expected == "PENDING") {
    FAIL("Golden baseline missing. Capture it with:\n"
         "  echo " << computed
         << " > " << BEEPBOX_GOLDEN_HASH_PATH);
  }
  CHECK(computed == expected);
}

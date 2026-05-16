#include "beepbox/Params.h"
#include <BeepingCoreLib_api.h>
#include <algorithm>

namespace beepbox {

// Minimum beep window in seconds. Mirrors the constant in beeping-core's
// scheduler (BEEPING_ComputeBeepSchedule rejects shorter windows). Not
// exposed in the public C API, so kept inline here for validation.
static constexpr float kMinBeepWindow = 2.3f;

ValidationResult validate(const Params& p) {
  ValidationResult r;

  // Key validation
  if (p.key.size() != 5) {
    r.ok = false;
    r.errors.push_back("Key must be exactly 5 base32 characters [0-9a-v]");
  } else {
    static constexpr char kDigits[] = "0123456789abcdefghijklmnopqrstuv";
    for (char c : p.key) {
      bool found = false;
      for (int i = 0; i < 32; ++i) {
        if (c == kDigits[i]) { found = true; break; }
      }
      if (!found) {
        r.ok = false;
        r.errors.push_back(std::string("Invalid character '") + c + "' in key. Use [0-9a-v]");
        break;
      }
    }
  }

  // Sample rate
  if (p.sampleRate < 22050.0f || p.sampleRate > 96000.0f) {
    r.ok = false;
    r.errors.push_back("Sample rate must be between 22050 and 96000 Hz");
  }

  // Duration
  float minDuration = std::max(p.startTime + kMinBeepWindow, kMinBeepWindow);
  if (p.duration < minDuration) {
    r.ok = false;
    r.errors.push_back("Duration too short. Minimum is " + std::to_string(minDuration) + "s (start + 2.3)");
  }
  if (p.duration > 86400.0f) {
    r.ok = false;
    r.errors.push_back("Duration too long. Maximum is 86400 seconds (24 hours)");
  }

  // Start time
  if (p.startTime < 0.0f) {
    r.ok = false;
    r.errors.push_back("Start time must be >= 0");
  }

  // Interval
  if (p.interval < kMinBeepWindow) {
    r.ok = false;
    r.errors.push_back("Interval too short. Minimum is 2.3 seconds");
  }

  // Volume beeps
  if (p.volumeBeepsdB < -60.0f || p.volumeBeepsdB > 12.0f) {
    r.warnings.push_back("Volume beeps clamped to [-60, 12] dB range");
  }

  // Volume program
  if (p.volumeProgramdB < -60.0f || p.volumeProgramdB > 12.0f) {
    r.warnings.push_back("Volume program clamped to [-60, 12] dB range");
  }

  return r;
}

int toCoreMode(Mode m) {
  switch (m) {
    case Mode::Audible:   return BEEPING_MODE_AUDIBLE;
    case Mode::Inaudible: return BEEPING_MODE_INAUDIBLE;
    case Mode::All:       return BEEPING_MODE_ALL;
  }
  return BEEPING_MODE_INAUDIBLE;
}

int toMixModeInt(MixMode m) {
  switch (m) {
    case MixMode::Default: return 0;
    case MixMode::Global:  return 1;
    case MixMode::Dynamic: return 2;
  }
  return 0;
}

bool parseMode(const std::string& s, Mode& out) {
  if (s == "audible")   { out = Mode::Audible;   return true; }
  if (s == "inaudible") { out = Mode::Inaudible;  return true; }
  if (s == "all")       { out = Mode::All;        return true; }
  return false;
}

bool parseMixMode(const std::string& s, MixMode& out) {
  if (s == "default") { out = MixMode::Default; return true; }
  if (s == "global")  { out = MixMode::Global;  return true; }
  if (s == "dynamic") { out = MixMode::Dynamic; return true; }
  return false;
}

} // namespace beepbox

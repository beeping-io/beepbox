#ifndef BEEPBOX_PARAMS_H
#define BEEPBOX_PARAMS_H

#include <string>
#include <vector>

namespace beepbox {

enum class Mode { Audible, Inaudible, All };

enum class MixMode { Default, Global, Dynamic };

struct Params {
  std::string key;             // 5 chars base32 [0-9a-v]
  Mode mode = Mode::Inaudible;
  float sampleRate = 44100.0f; // 22050–96000
  float duration = 2.3f;       // seconds (min 2.3)
  float startTime = 0.0f;      // offset for first beep
  float interval = 2.3f;       // seconds between beeps (min 2.3)
  float volumeBeepsdB = -3.0f; // beep gain in dB (-60..12)

  // Mixer (only used when hostAudio is provided)
  MixMode mixMode = MixMode::Default;
  float volumeProgramdB = 0.0f; // program gain in dB

  // Flags
  bool loudness = false;        // compute LKFS + true peak
};

struct ValidationResult {
  bool ok = true;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
};

/// Validate params and return errors/warnings.
ValidationResult validate(const Params& p);

/// Map Mode enum to beeping-core integer constant.
int toCoreMode(Mode m);

/// Map MixMode enum to integer (0/1/2).
int toMixModeInt(MixMode m);

/// Parse mode string ("audible"/"inaudible"/"all") to enum. Returns false on invalid.
bool parseMode(const std::string& s, Mode& out);

/// Parse mixmode string ("default"/"global"/"dynamic") to enum. Returns false on invalid.
bool parseMixMode(const std::string& s, MixMode& out);

} // namespace beepbox

#endif // BEEPBOX_PARAMS_H

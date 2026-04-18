#include "beepbox/Params.h"
#include "beepbox/Base32.h"
#include "beepbox/Scheduler.h"
#include "beepbox/Generator.h"
#include "beepbox/LoudnessStats.h"
#include "Mixer.h"
#include "CliParser.hxx"

#include <BeepingCoreLib_api.h>
#include <sndfile.h>

#include <iostream>
#include <cstring>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

using namespace beepbox;

static void printHelp() {
  std::cerr << R"(
BeepBox — encode data over sound

USAGE:
  BeepBox -k <KEY> -o <OUTPUT.wav> [options]

REQUIRED:
  -k, --key <KEY>              5 chars base32 [0-9 a-v]
  -o, --output <PATH>          Output WAV file

MODE:
  -m, --mode <MODE>            audible | inaudible | all (default: inaudible)

GENERATION:
  -d, --duration <seconds>     Total duration, min 2.3 (default: 2.3)
  -s, --start <seconds>        First beep offset, >=0 (default: 0.0)
  -i, --interval <seconds>     Repetition, min 2.3 (default: 2.3)
  -r, --samplerate <Hz>        22050-96000 (default: 44100). Ignored with --file

MIXER (requires --file):
  -f, --file <PATH>            Input WAV to mix beeps into
  -x, --mixmode <MODE>         default | global | dynamic (default: default)
  -v, --volume-beeps <dB>      Beep gain, -60..12 (default: -3)
  -p, --volume-program <dB>    Program gain (default: 0)

ANALYSIS:
  -l, --loudness               Enable LKFS + true peak statistics

UTILITIES:
  -n, --dry-run                Validate and show plan, no audio generated
  -h, --help                   Show this help
)";
}

static std::string modeToString(Mode m) {
  switch (m) {
    case Mode::Audible:   return "audible";
    case Mode::Inaudible: return "inaudible";
    case Mode::All:       return "all";
  }
  return "unknown";
}

static std::string mixModeToString(MixMode m) {
  switch (m) {
    case MixMode::Default: return "default";
    case MixMode::Global:  return "global";
    case MixMode::Dynamic: return "dynamic";
  }
  return "unknown";
}

int main(int argc, char** argv) {
  CliParser cli;
  cli.addOption("h", "help", CliParser::CLI_NONE, true, "", "Show help", "");
  cli.addOption("m", "mode", CliParser::CLI_STRING, true, "mode", "audible|inaudible|all", "inaudible");
  cli.addOption("f", "file", CliParser::CLI_STRING, true, "path", "Input WAV to mix with", "");
  cli.addOption("k", "key", CliParser::CLI_STRING, false, "key", "5 chars base32", "");
  cli.addOption("d", "duration", CliParser::CLI_FLOAT, true, "secs", "Duration", "2.3");
  cli.addOption("i", "interval", CliParser::CLI_FLOAT, true, "secs", "Interval", "2.3");
  cli.addOption("s", "start", CliParser::CLI_FLOAT, true, "secs", "Start offset", "0");
  cli.addOption("o", "output", CliParser::CLI_STRING, false, "path", "Output WAV", "");
  cli.addOption("x", "mixmode", CliParser::CLI_STRING, true, "mode", "default|global|dynamic", "default");
  cli.addOption("v", "volume-beeps", CliParser::CLI_FLOAT, true, "dB", "Beep gain", "-3.0");
  cli.addOption("p", "volume-program", CliParser::CLI_FLOAT, true, "dB", "Program gain", "0.0");
  cli.addOption("r", "samplerate", CliParser::CLI_FLOAT, true, "Hz", "Sample rate", "44100.0");
  cli.addOption("l", "loudness", CliParser::CLI_NONE, true, "", "Loudness stats", "");
  cli.addOption("n", "dry-run", CliParser::CLI_NONE, true, "", "Dry run", "");

  if (!cli.parse(argc, argv)) {
    printHelp();
    return 1;
  }

  if (cli.hasOption("h")) {
    printHelp();
    return 0;
  }

  // --- Parse CLI into Params ---
  Params p;
  p.key = cli.getOptionAsString("k", "");

  std::string modeStr = cli.getOptionAsString("m", "inaudible");
  if (!parseMode(modeStr, p.mode)) {
    std::cerr << "Error: invalid mode '" << modeStr << "'. Use: audible, inaudible, all\n";
    return 1;
  }

  p.sampleRate = cli.getOptionAsFloat("r", 44100.0f);
  p.duration = cli.getOptionAsFloat("d", 2.3f);
  p.startTime = cli.getOptionAsFloat("s", 0.0f);
  p.interval = cli.getOptionAsFloat("i", 2.3f);
  p.volumeBeepsdB = cli.getOptionAsFloat("v", -3.0f);
  p.volumeProgramdB = cli.getOptionAsFloat("p", 0.0f);
  p.loudness = cli.hasOption("l");

  std::string mixModeStr = cli.getOptionAsString("x", "default");
  if (!parseMixMode(mixModeStr, p.mixMode)) {
    std::cerr << "Error: invalid mixmode '" << mixModeStr << "'. Use: default, global, dynamic\n";
    return 1;
  }

  std::string inputFile = cli.getOptionAsString("f", "");
  std::string outputFile = cli.getOptionAsString("o", "");

  std::vector<std::string> rawArgs(argv + 1, argv + argc);
  auto flagProvided = [&](const std::string& sf, const std::string& lf) {
    for (const auto& a : rawArgs) {
      if (a == "-" + sf || a == "--" + lf) return true;
    }
    return false;
  };
  bool dryRun = flagProvided("n", "dry-run");

  if (outputFile.empty()) {
    std::cerr << "Error: output file is required (--output)\n";
    return 1;
  }

  // --- Validate ---
  auto vr = validate(p);
  for (const auto& w : vr.warnings) std::cerr << "Warning: " << w << "\n";
  if (!vr.ok) {
    for (const auto& e : vr.errors) std::cerr << "Error: " << e << "\n";
    return 1;
  }

  // --- No input file: generate beeps only ---
  if (inputFile.empty()) {
    int beepCount = computeBeepCount(p.duration, p.startTime, p.interval);
    auto schedule = computeBeepSchedule(p.duration, p.startTime, p.interval);

    if (beepCount <= 0) {
      std::cerr << "Error: duration too short for any beeps\n";
      return 1;
    }

    if (dryRun) {
      std::cout << "Dry-run plan:\n"
                << "  key:        " << p.key << "\n"
                << "  mode:       " << modeToString(p.mode) << "\n"
                << "  duration:   " << p.duration << " s\n"
                << "  interval:   " << p.interval << " s\n"
                << "  start:      " << p.startTime << " s\n"
                << "  samplerate: " << p.sampleRate << " Hz\n"
                << "  output:     " << outputFile << "\n"
                << "  beeps:      " << beepCount << "\n";
      return 0;
    }

    auto result = generateBeeps(p);

    SF_INFO sfinfo{};
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfo.channels = 1;
    sfinfo.samplerate = static_cast<int>(p.sampleRate);

    SNDFILE* out = sf_open(outputFile.c_str(), SFM_WRITE, &sfinfo);
    if (!out) {
      std::cerr << "Error: cannot create " << outputFile << "\n";
      return 1;
    }
    sf_write_float(out, result.samples.data(), static_cast<sf_count_t>(result.samples.size()));
    sf_close(out);

    std::cout << "Generated " << result.beepsGenerated << " beep(s) → " << outputFile << "\n";
  }
  // --- With input file: mix beeps into host audio ---
  else {
    SF_INFO sfinfoIn{};
    SNDFILE* in = sf_open(inputFile.c_str(), SFM_READ, &sfinfoIn);
    if (!in) {
      std::cerr << "Error: cannot open " << inputFile << "\n";
      return 1;
    }

    float fileSampleRate = static_cast<float>(sfinfoIn.samplerate);
    long nFrames = sfinfoIn.frames;
    int nch = sfinfoIn.channels;

    // Read input to per-channel buffers
    int bufSamples = 4096;
    std::vector<float> interleaved(bufSamples * nch);
    std::vector<std::vector<float>> channels(nch, std::vector<float>(nFrames, 0.0f));

    long readTotal = 0;
    while (readTotal < nFrames * nch) {
      int count = static_cast<int>(sf_read_float(in, interleaved.data(), bufSamples * nch));
      for (int ch = 0; ch < nch; ++ch) {
        for (int i = 0; i < count / nch; ++i) {
          channels[ch][(readTotal / nch) + i] = interleaved[i * nch + ch];
        }
      }
      readTotal += count;
    }
    sf_close(in);

    float hostDuration = static_cast<float>(nFrames) / fileSampleRate;
    int beepCount = computeBeepCount(hostDuration, p.startTime, p.interval);

    if (beepCount <= 0) {
      std::cerr << "Error: input file too short for any beeps\n";
      return 1;
    }

    if (dryRun) {
      std::cout << "Dry-run plan:\n"
                << "  key:        " << p.key << "\n"
                << "  mode:       " << modeToString(p.mode) << "\n"
                << "  duration:   " << hostDuration << " s (from input file)\n"
                << "  interval:   " << p.interval << " s\n"
                << "  start:      " << p.startTime << " s\n"
                << "  samplerate: " << fileSampleRate << " Hz\n"
                << "  input:      " << inputFile << "\n"
                << "  output:     " << outputFile << "\n"
                << "  mixmode:    " << mixModeToString(p.mixMode) << "\n"
                << "  beeps:      " << beepCount << "\n";
      return 0;
    }

    auto beepResult = generateBeepsForHost(p, static_cast<int>(nFrames), fileSampleRate);

    // Mix using Mixer
    Mixer mixer;
    mixer.setBeepLevel(p.volumeBeepsdB);
    mixer.setMinBeepLevel(-20.0f);
    mixer.setProgramLevel(p.volumeProgramdB);
    mixer.setMode(toMixModeInt(p.mixMode));
    mixer.setUseNormalize(false);

    // Build raw pointer arrays for Mixer API
    std::vector<const float*> inPtrs(nch);
    std::vector<float*> outPtrs(nch);
    std::vector<std::vector<float>> mixedChannels(nch, std::vector<float>(nFrames, 0.0f));
    for (int ch = 0; ch < nch; ++ch) {
      inPtrs[ch] = channels[ch].data();
      outPtrs[ch] = mixedChannels[ch].data();
    }

    mixer.mix(inPtrs.data(), static_cast<int>(nFrames), nch, fileSampleRate,
              beepResult.samples.data(), outPtrs.data());

    // Write output
    SF_INFO sfinfoOut = sfinfoIn;
    sfinfoOut.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    SNDFILE* out = sf_open(outputFile.c_str(), SFM_WRITE, &sfinfoOut);
    if (!out) {
      std::cerr << "Error: cannot create " << outputFile << "\n";
      return 1;
    }

    std::vector<float> outInterleaved(bufSamples * nch);
    long written = 0;
    while (written < nFrames) {
      int toWrite = std::min(static_cast<long>(bufSamples), nFrames - written);
      for (int ch = 0; ch < nch; ++ch) {
        for (int i = 0; i < toWrite; ++i) {
          outInterleaved[i * nch + ch] = mixedChannels[ch][written + i];
        }
      }
      sf_write_float(out, outInterleaved.data(), toWrite * nch);
      written += toWrite;
    }
    sf_close(out);

    std::cout << "Mixed " << beepResult.beepsGenerated << " beep(s) into " << inputFile
              << " → " << outputFile << "\n";
  }

  // --- Loudness stats ---
  if (p.loudness) {
    double lufs = test_global_loudness(outputFile.c_str());
    double tp = test_true_peak(outputFile.c_str());
    std::cout << "Loudness: " << lufs << " LKFS, True Peak: " << tp << " dB\n";
  }

  return 0;
}

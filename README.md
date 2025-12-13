# BeepBox

BeepBox is Beeping’s CLI tool to generate or embed acoustic marks (beeps) into WAV files. It relies on **Beeping Core** to synthesize marks and supports audible, hidden, non-audible, and custom modes. It transports identifier data through audio in a robust way.

## Installation / Build
- Dependencies: CMake, C++ compiler, `libsndfile` (system), `libebur128` (FetchContent), Beeping Core (FetchContent).
- Steps:
  ```bash
  cmake -S . -B build
  cmake --build build
  cmake --install build   # optional, installs into ./bin and ./lib in the repo
  ```
- Binary: `build/BeepBox` (or `bin/BeepBox` after install).

## Basic usage
Minimal command (generates 1 beep by default):
```bash
./build/BeepBox -k 00001 -o output.wav
```
Defaults: `duration=2.3s`, `interval=2.3s`, `start=0`, mode `non-audible` (2), samplerate 44100 Hz. If only one beep fits, it is placed at start and `interval` is ignored.

## CLI parameters (grouped)
**Required**
- `-k, --key` (string): 5 chars base32 `[0-9a-v]` (required).
- `-o, --output` (string): Output WAV path (required).

**Optional (generation)**
- `-m, --mode <0|1|2|3>`: 0 audible | 1 hidden | 2 non-audible (default) | 3 custom.
- `-d, --duration <s>`: Total duration. Min `2.3`; must satisfy `start + 2.3 <= duration`. Default `2.3`.
- `-s, --start <s>`: First beep start (>=0). Default `0.0`.
- `-i, --interval <s>`: Separation between beeps. Min `2.3`. Default `2.3`. If only one beep fits, it’s ignored with warning.
- `-r, --samplerate <Hz>`: 44100 or 48000 when generating without `--file`. Ignored if `--file` is present. Default `44100`.
- `-n, --dry-run`: Validate and show plan (beeps/timestamps) without generating audio or writing files.

**Mixer (requires `--file`)**
- `-f, --file <wav>`: Input WAV (44.1k/48k PCM16) to mix.
- `-x, --mixmode <0|1|2>`: 0 DefaultLevel | 1 GlobalLevel | 2 DynamicLevel. Requires `--file`.
- `-p, --volumeprogram <dB>`: Program gain. Default `0`. Requires `--file`.
- `-l, --loudnessstatistics <0|1>`: Loudness stats (default `0`); applies to output; typically used when mixing.

**Custom mode (only `mode=3`)**
- `--bf, --basefreq <Hz>`: Base frequency. Default `12000`. Must be >0.
- `--ts, --tonesseparation <int>`: Tone separation (>=1). Default `1`.

**Advanced (synthesis and levels)**
- `-sm, --synthmode <0|1>`: 0 off | 1 r2d2. Clamped [0,1]. Default `0`.
- `-sv, --synthvolume <dB>`: Synth volume relative to beeps. Clamped [-60,12]. Default `0`.
- `-v, --volumebeeps <dB>`: Beeps gain. Clamped [-60,12]. Default `-3`.
- `-h, --help`: Show help and examples.

## Conceptual rules
- One beep lasts at least `2.3s`.
- First beep starts at `--start` (default 0). Must satisfy `start + 2.3 <= duration`.
- `--interval` sets spacing between beeps; must be >=2.3. If only one beep fits, `interval` is ignored with warning.
- For mixing (`--mixmode`, `--volumeprogram`) you must provide `--file`; `mixmode` without file is an error, `volumeprogram` without file is ignored.
- `--samplerate` is only used when generating from scratch; with `--file` the input WAV samplerate is used.
- Out-of-range values clamp with warnings (volumes, synthmode). Errors block execution; warnings adjust/ignore and continue.

## Examples
- Single default beep:
  ```bash
  BeepBox -k 00001 -o beep.wav
  ```
- Multiple beeps on generated audio:
  ```bash
  BeepBox -k 00001 -o long.wav -d 15 -i 3 -s 0
  ```
- Mix into existing WAV:
  ```bash
  BeepBox -k 00001 -f input.wav -o mixed.wav -m 2 -i 4 -x 1 -v -6 -p 0
  ```
- Custom mode (freq and separation):
  ```bash
  BeepBox -k 00001 -o custom.wav -m 3 -bf 15000 -ts 10 -d 10 -i 3
  ```
- Dry-run (plan only):
  ```bash
  BeepBox -n -k 00001 -o plan.wav -d 10 -i 2.5
  ```

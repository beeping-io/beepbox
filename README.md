# BeepBox

BeepBox is Beeping's **HTTP server and CLI tool** to generate or embed acoustic marks (beeps) into WAV files. It relies on **Beeping Core** to synthesize marks and supports audible, hidden, non-audible, and custom modes. It transports identifier data through audio in a robust way.

<!-- Identity -->
![License](https://img.shields.io/badge/License-Apache_2.0-blue)
![status](https://img.shields.io/badge/status-early_development-orange)
![platform](https://img.shields.io/badge/platform-Beeping-purple)
![conventional commits](https://img.shields.io/badge/conventional_commits-1.0.0-yellow)

<!-- Tech stack -->
![C++](https://img.shields.io/badge/C++-20-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.25+-064F8C?logo=cmake&logoColor=white)
![Drogon](https://img.shields.io/badge/Drogon-HTTP_framework-2D9CDB)

<!-- CI -->
[![Build & Test](https://github.com/beeping-io/beepbox/actions/workflows/ci.yml/badge.svg)](https://github.com/beeping-io/beepbox/actions/workflows/ci.yml)
[![Docker](https://github.com/beeping-io/beepbox/actions/workflows/docker-build.yml/badge.svg)](https://github.com/beeping-io/beepbox/actions/workflows/docker-build.yml)
[![Deploy](https://github.com/beeping-io/beepbox/actions/workflows/deploy.yml/badge.svg)](https://github.com/beeping-io/beepbox/actions/workflows/deploy.yml)
[![OpenAPI Clients](https://github.com/beeping-io/beepbox/actions/workflows/openapi-clients.yml/badge.svg)](https://github.com/beeping-io/beepbox/actions/workflows/openapi-clients.yml)

The same repo produces two consumption shapes:

- **HTTP server** (`beepbox_server`) — REST API at `/v1/encode` and `/v1/decode`, used by SDK wrappers (Android, iOS, web) and any client over the wire. Drogon-based, with API-key auth via a Firestore-backed `HttpKeyStore`, CORS allowlist, rate limiting, OpenTelemetry tracing and Prometheus metrics.
- **CLI** (`BeepBox`) — host-side WAV generation/mixing for batch jobs, demos and tests. The flags below cover the CLI surface.

## Installation / Build
- Dependencies: CMake, C++ compiler, `libsndfile` (system), `libebur128` (FetchContent), Beeping Core (FetchContent), Drogon (FetchContent, server only).
- Steps:
  ```bash
  cmake -S . -B build
  cmake --build build
  cmake --install build   # optional, installs into ./bin and ./lib in the repo
  ```
- Binaries: `build/BeepBox` (CLI) and `build/beepbox_server` (HTTP server).

## HTTP server

Run the server locally:

```bash
./build/beepbox_server  # binds 0.0.0.0:8080 by default
```

Smoke test (audible WAV at `/v1/encode`, requires a valid API key):

```bash
curl -X POST -H "Authorization: Bearer bk_..." \
  -H "Content-Type: application/json" \
  -d '{"key":"abcde","mode":"audible"}' \
  http://localhost:8080/v1/encode --output beep.wav
```

Endpoints, request/response shapes and error codes are documented in:

- [`docs/api/reference.md`](docs/api/reference.md) — human-readable reference
- [`docs/api/openapi.yaml`](docs/api/openapi.yaml) — OpenAPI 3.0 spec (used by the `openapi-clients` workflow to generate SDK stubs)

Operational docs live under [`docs/ops/`](docs/ops/) (deploy, infrastructure, environments, supply-chain) and [`docs/observability/`](docs/observability/) (metrics, tracing).

## Basic usage (CLI)
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

## Community

### Chat & support

- 💬 **Discord** — [chat with the team and other developers](https://discord.gg/beeping)
- 💼 **Slack** — coming soon (will be cross-bridged with Discord)
- 🐛 **Issues** — [github.com/beeping-io/beepbox/issues](https://github.com/beeping-io/beepbox/issues) for bugs and feature requests

### Follow us

Social handles coming soon — see the issue tracker for status.

- 🐦 X (Twitter) · 💼 LinkedIn · 📸 Instagram · 🎵 TikTok · 📺 YouTube · 🐘 Mastodon · 🦋 Bluesky

### Project governance

- 🛡️ **Security** — formal `SECURITY.md` disclosure policy is on the way; in the meantime, please email security at beeping dot io for vulnerability reports
- 🤝 **Contributing** — formal `CONTRIBUTING.md` is on the way; the project uses conventional commits, semantic-release via `release-please`, and PRs against `develop`

Beeping is built in the open. BeepBox is the HTTP server and CLI; the C++20
synthesis/decoding library lives in [`beeping-core`](https://github.com/beeping-io/beeping-core),
and SDK wrappers + the marketing site live in the sibling repos of the
[`beeping-io`](https://github.com/beeping-io) org.

## 💬 Community

Have a question, idea or showcase? Three places to talk:

- **GitHub Discussions** — async forum, indexable: <https://github.com/beeping-io/.github/discussions>
- **Slack** — real-time chat: [join the workspace](https://join.slack.com/t/beeping-io/shared_invite/zt-3yv2kc6qs-tWxx1AViHgdEembSPqm26Q)
- **Discord** — same channels, Discord flavour: <https://discord.gg/XNPXdZK7>

Code of Conduct (Contributor Covenant 2.1): <https://github.com/beeping-io/.github/blob/main/CODE_OF_CONDUCT.md>

## License

[Apache-2.0](LICENSE)

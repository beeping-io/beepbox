# Changelog

## [0.2.0](https://github.com/beeping-io/beepbox/compare/v0.1.0...v0.2.0) (2026-10-22)


### ✨ Features

* **api:** BEEP-93 document /v1/decode response fields and expand round-trip tests ([d3d786c](https://github.com/beeping-io/beepbox/commit/d3d786ccbe0d0bf2331e2e41712aa4006bf4e3fc))
* **api:** BEEP-93 document /v1/decode response fields and expand round-trip tests ([63c9f0b](https://github.com/beeping-io/beepbox/commit/63c9f0b268a429564cc46bb0f7879a89de1ce63a))
* **server:** BEEP-92 upgrade to beeping-core v0.8.2 and expose /health edge route ([9a7d87b](https://github.com/beeping-io/beepbox/commit/9a7d87b13d2d2e38efe7dfa9ec09c6f7e2069ad2))
* **server:** BEEP-92 upgrade to beeping-core v0.8.2 and expose /health edge route ([18e63f2](https://github.com/beeping-io/beepbox/commit/18e63f2ce13e21f0ad5dd6b1aa6cfb81f69c5f53))
* **toolchain:** BEE-2440 configure lefthook git hooks ([e53c1c5](https://github.com/beeping-io/beepbox/commit/e53c1c5e82952e75121f46493eae56b4ff5e1c87))


### 🐛 Bug Fixes

* **smoke:** BEEP-92 align external health check with GFE /healthz edge reservation ([b6bd2f4](https://github.com/beeping-io/beepbox/commit/b6bd2f4fbccca2c42b51c3cfef59942c6d6fa172))
* **smoke:** BEEP-92 align external health check with GFE /healthz edge reservation ([661ae65](https://github.com/beeping-io/beepbox/commit/661ae65004737c408ab6f4b429264dd851bd87d9))


### 📚 Documentation

* **agy:** standardize GEMINI.md markdown formatting ([c1b166c](https://github.com/beeping-io/beepbox/commit/c1b166cb098d094606437b9898b48d9ddad8e511))
* BEE-2404 add native GEMINI.md for beepbox ([1acc472](https://github.com/beeping-io/beepbox/commit/1acc472ec6e5e2e10b9e77d04b53ccb6d051baf7))
* **beepbox:** sanitize legacy Linear references in docs ([dfe1596](https://github.com/beeping-io/beepbox/commit/dfe1596035a60283195770b2670abeab54f50b1a))
* **pending:** capture v0.1.0 deploy follow-ups (auth keys, GHCR build, release-trigger) ([#11](https://github.com/beeping-io/beepbox/issues/11)) ([6bef33d](https://github.com/beeping-io/beepbox/commit/6bef33d1e50d4673891ad180796e5d3047141677))
* **readme:** link the 3 community channels (GitHub Discussions, Slack, Discord) ([#17](https://github.com/beeping-io/beepbox/issues/17)) ([aafa4eb](https://github.com/beeping-io/beepbox/commit/aafa4eb55793f9c751aacbdb881ecb8a97cce964))

## [0.1.0](https://github.com/beeping-io/beepbox/compare/v0.0.0...v0.1.0) (2026-05-16)


### ✨ Features

* **server:** BEE-1794 enable CORS for browser clients ([#6](https://github.com/beeping-io/beepbox/issues/6)) ([96cf4a8](https://github.com/beeping-io/beepbox/commit/96cf4a8746afd5063af5a78859cc40db76aad74c))
* **server:** BEE-1800 deploy current image to prod with prod-only CORS whitelist (<https://beeping.io>) — squashed in [#8](https://github.com/beeping-io/beepbox/issues/8) ([ff749c5](https://github.com/beeping-io/beepbox/commit/ff749c5))
* **server:** BEE-2239 use beeping-core scheduler API (drop duplicate Scheduler.cpp) + expose `code` and `timestampSec` in /v1/decode (additive, legacy `decoded` preserved). Encoder now emits exactly `floor(duration × sampleRate)` samples, beeps placed at timestamp-aligned positions — squashed in [#9](https://github.com/beeping-io/beepbox/issues/9) ([31c055c](https://github.com/beeping-io/beepbox/commit/31c055c))


### 🐛 Bug Fixes

* **ci:** BEE-1674 point release-please at develop — repo has no main branch, workflow had never fired since registration in [#5](https://github.com/beeping-io/beepbox/issues/5) ([31c055c](https://github.com/beeping-io/beepbox/commit/31c055c))


### 📚 Documentation

* **pending:** capture WIF + /healthz GFE follow-ups from BEE-1794 deploy ([19f13f2](https://github.com/beeping-io/beepbox/commit/19f13f2188a726c5fb5fb978bf5f836c8b670157))
* **repo:** BEE-1798 reorganize docs tree + add environments snapshot ([#7](https://github.com/beeping-io/beepbox/issues/7)) ([eb39fce](https://github.com/beeping-io/beepbox/commit/eb39fce83336f6397daca137c2ec0617cb196a97))


### 🔐 Phase 2 prod hardening (squashed in [#8](https://github.com/beeping-io/beepbox/issues/8))

The following infra/security tasks were not picked up by Conventional Commits parsing (squash commit used `milestone(...)` type) but landed in this release:

* BEE-1799 — chore(infra): terraformize prod env + GCS backend (dev/prod parity)
* BEE-1801 — security(cf): require Firebase Auth on `generateApiKey` + `revokeApiKey` (lock down public invocation)
* BEE-1802 — chore(test): mint test API keys for SDK E2E suites (dev + prod) + sync to GCP
* BEE-1803 — infra(ci): wire Workload Identity Federation for multi-env deploys (closes pending-001)
* BEE-1804 — infra(domain): configure beepbox.beeping.io (prod) + beepbox-dev.beeping.io (dev)

## Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## Unreleased
### Added
* Opción `--dry-run` para validar parámetros y mostrar el plan sin generar audio.
* Defaults coherentes documentados (p. ej. duración por defecto 2.3s).
* Ayuda del CLI ampliada con descripciones y ejemplos.

### Changed
* Validación conceptual de intervalos/duración y coherencia entre parámetros.
* Validaciones del CLI más estrictas y con mensajes claros.

### Fixed
* Combinaciones de parámetros inconsistentes o sin sentido ahora se detectan.
* Defaults y comportamientos previamente implícitos ahora están documentados.

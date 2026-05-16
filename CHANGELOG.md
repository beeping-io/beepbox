# Changelog

## [0.1.0](https://github.com/beeping-io/beepbox/compare/v0.0.0...v0.1.0) (2026-05-16)


### ✨ Features

* **server:** BEE-1794 enable CORS for browser clients ([#6](https://github.com/beeping-io/beepbox/issues/6)) ([96cf4a8](https://github.com/beeping-io/beepbox/commit/96cf4a8746afd5063af5a78859cc40db76aad74c))


### 📚 Documentation

* **pending:** capture WIF + /healthz GFE follow-ups from BEE-1794 deploy ([19f13f2](https://github.com/beeping-io/beepbox/commit/19f13f2188a726c5fb5fb978bf5f836c8b670157))
* **repo:** BEE-1798 reorganize docs tree + add environments snapshot ([#7](https://github.com/beeping-io/beepbox/issues/7)) ([eb39fce](https://github.com/beeping-io/beepbox/commit/eb39fce83336f6397daca137c2ec0617cb196a97))

## Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## Unreleased
### Added
- Opción `--dry-run` para validar parámetros y mostrar el plan sin generar audio.
- Defaults coherentes documentados (p. ej. duración por defecto 2.3s).
- Ayuda del CLI ampliada con descripciones y ejemplos.

### Changed
- Validación conceptual de intervalos/duración y coherencia entre parámetros.
- Validaciones del CLI más estrictas y con mensajes claros.

### Fixed
- Combinaciones de parámetros inconsistentes o sin sentido ahora se detectan.
- Defaults y comportamientos previamente implícitos ahora están documentados.

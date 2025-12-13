#!/usr/bin/env bash
set -euo pipefail

ID="${1:-00001}"
OUT_DIR="beeps"
DURATION="5.2"

mkdir -p "${OUT_DIR}"

# Ajusta esta ruta si tu binario vive en build/Debug o build/Release
BIN="${BIN:-./build/BeepBox}"

"${BIN}" -m 0 -k "${ID}" -o "${OUT_DIR}/${ID}-audible.wav"     -d "${DURATION}"
"${BIN}" -m 1 -k "${ID}" -o "${OUT_DIR}/${ID}-hidden.wav"      -d "${DURATION}"
"${BIN}" -m 2 -k "${ID}" -o "${OUT_DIR}/${ID}-non-audible.wav" -d "${DURATION}"
"${BIN}" -m 3 -k "${ID}" -o "${OUT_DIR}/${ID}-custom.wav"      -d "${DURATION}"
echo "OK: WAVs generados en ${OUT_DIR}/ para ID=${ID}"
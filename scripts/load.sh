#!/usr/bin/env bash
# Run k6 load tests against beepbox-server
# Usage: ./scripts/load.sh [BASE_URL]
set -euo pipefail

BASE_URL="${1:-https://beepbox-server-ai7n45q5lq-ew.a.run.app}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if ! command -v k6 &>/dev/null; then
  echo "k6 not installed. Install with: brew install k6"
  exit 1
fi

echo "=== Running k6 load tests against $BASE_URL ==="
k6 run --env "BASE_URL=${BASE_URL}" "${SCRIPT_DIR}/../k6/load-test.js"

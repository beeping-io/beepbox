#!/usr/bin/env bash
# Post-deploy smoke test for beepbox-server
# Usage: ./scripts/smoke.sh [BASE_URL] [API_KEY]
set -euo pipefail

BASE_URL="${1:-https://beepbox-server-ai7n45q5lq-ew.a.run.app}"
API_KEY="${2:-}"
PASS=0
FAIL=0

auth_header() {
  if [ -n "$API_KEY" ]; then
    echo "-H" "Authorization: Bearer $API_KEY"
  fi
}

check() {
  local name="$1" expected="$2"
  shift 2
  local code
  code=$(curl -s -o /dev/null -w '%{http_code}' --max-time 30 "$@" 2>/dev/null || echo "000")

  if [ "$code" = "$expected" ]; then
    echo "  PASS  $name → HTTP $code"
    PASS=$((PASS + 1))
  else
    echo "  FAIL  $name → HTTP $code (expected $expected)"
    FAIL=$((FAIL + 1))
  fi
}

echo "=== Smoke tests: $BASE_URL ==="
[ -n "$API_KEY" ] && echo "  (using API key)" || echo "  (no API key — /v1/* may return 401)"
echo ""

# 1. Readiness probe (no auth needed)
check "/readyz" 200 -X GET "${BASE_URL}/readyz"

# 2. Version endpoint (no auth needed)
check "/version" 200 -X GET "${BASE_URL}/version"

# 3. Metrics (no auth needed)
check "/metrics" 200 -X GET "${BASE_URL}/metrics"

# 4. Encode — valid request
if [ -n "$API_KEY" ]; then
  check "/v1/encode" 200 \
    -X POST "${BASE_URL}/v1/encode" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $API_KEY" \
    -d '{"key":"a1b2c","mode":"inaudible","duration":2.3}'
else
  # Without API key, expect 401 if auth is enabled, 200 if disabled
  check "/v1/encode (no auth)" 200 \
    -X POST "${BASE_URL}/v1/encode" \
    -H "Content-Type: application/json" \
    -d '{"key":"a1b2c","mode":"inaudible","duration":2.3}'
fi

# 5. Decode — empty body (expect 400 with auth, 401 without)
if [ -n "$API_KEY" ]; then
  check "/v1/decode (empty → 400)" 400 \
    -X POST "${BASE_URL}/v1/decode" \
    -H "Content-Type: audio/wav" \
    -H "Authorization: Bearer $API_KEY"
else
  check "/v1/decode (no auth)" 400 \
    -X POST "${BASE_URL}/v1/decode" \
    -H "Content-Type: audio/wav"
fi

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="

if [ "$FAIL" -gt 0 ]; then
  echo "SMOKE TESTS FAILED"
  exit 1
fi

echo "ALL SMOKE TESTS PASSED"

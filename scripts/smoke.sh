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

# 1. Liveness probe (/health works on all endpoints; /healthz is intercepted by
#    Google Frontend on Cloud Run / Firebase Hosting and only reachable locally)
check "/health" 200 -L -X GET "${BASE_URL}/health"
if [[ "${BASE_URL}" == *"localhost"* || "${BASE_URL}" == *"127.0.0.1"* ]]; then
  check "/healthz" 200 -L -X GET "${BASE_URL}/healthz"
fi

# 2. Readiness probe (no auth needed)
check "/readyz" 200 -X GET "${BASE_URL}/readyz"

# 3. Version endpoint + verify dynamic core version (no legacy 20220426 literal)
check "/version" 200 -X GET "${BASE_URL}/version"
VERSION_BODY=$(curl -s --max-time 30 "${BASE_URL}/version" 2>/dev/null || echo "{}")
if echo "$VERSION_BODY" | grep -q '1.0.0 \[20220426\]'; then
  echo "  FAIL  /version core still reports legacy 1.0.0 [20220426]: $VERSION_BODY"
  FAIL=$((FAIL + 1))
elif echo "$VERSION_BODY" | grep -q '"core"'; then
  echo "  PASS  /version core is dynamic → $VERSION_BODY"
  PASS=$((PASS + 1))
else
  echo "  FAIL  /version missing core field: $VERSION_BODY"
  FAIL=$((FAIL + 1))
fi

# 4. Metrics (no auth needed)
check "/metrics" 200 -X GET "${BASE_URL}/metrics"

# 5. Encode & Decode round-trip
if [ -n "$API_KEY" ]; then
  TMP_WAV=$(mktemp /tmp/beepbox_smoke_XXXXXX.wav)
  ENC_CODE=$(curl -s -o "$TMP_WAV" -w '%{http_code}' --max-time 30 \
    -X POST "${BASE_URL}/v1/encode" \
    -H "Content-Type: application/json" \
    -H "Authorization: Bearer $API_KEY" \
    -d '{"key":"a1b2c","mode":"audible","duration":2.3}' 2>/dev/null || echo "000")
  if [ "$ENC_CODE" = "200" ]; then
    echo "  PASS  /v1/encode (with auth) → HTTP 200"
    PASS=$((PASS + 1))
    DEC_BODY=$(mktemp /tmp/beepbox_smoke_dec_XXXXXX.json)
    DEC_CODE=$(curl -s -o "$DEC_BODY" -w '%{http_code}' --max-time 30 \
      -X POST "${BASE_URL}/v1/decode" \
      -H "Content-Type: audio/wav" \
      -H "Authorization: Bearer $API_KEY" \
      --data-binary "@$TMP_WAV" 2>/dev/null || echo "000")
    if [ "$DEC_CODE" = "200" ] && grep -q 'a1b2c' "$DEC_BODY"; then
      echo "  PASS  /v1/decode round-trip (with auth) → HTTP 200 ($(cat "$DEC_BODY"))"
      PASS=$((PASS + 1))
    else
      echo "  FAIL  /v1/decode round-trip → HTTP $DEC_CODE ($(cat "$DEC_BODY" 2>/dev/null || true))"
      FAIL=$((FAIL + 1))
    fi
    rm -f "$DEC_BODY"
  else
    echo "  FAIL  /v1/encode (with auth) → HTTP $ENC_CODE (expected 200)"
    FAIL=$((FAIL + 1))
  fi
  rm -f "$TMP_WAV"
else
  # Auth required (BEE-1687) — 401 is the correct response without a key
  check "/v1/encode (no auth → 401)" 401 \
    -X POST "${BASE_URL}/v1/encode" \
    -H "Content-Type: application/json" \
    -d '{"key":"a1b2c","mode":"inaudible","duration":2.3}'
  check "/v1/decode (no auth → 401)" 401 \
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

#!/usr/bin/env bash
# Test: beepbox-server shuts down cleanly on SIGTERM within 10s
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SERVER="${SCRIPT_DIR}/../build/beepbox-server"
PORT=18090  # avoid conflict with dev server

if [ ! -x "$SERVER" ]; then
  echo "SKIP: beepbox-server not built at $SERVER"
  exit 0
fi

# Start server on test port (override via env not supported, so we check 8080)
# Use port 8080 if nothing is listening
if lsof -ti:8080 >/dev/null 2>&1; then
  echo "SKIP: port 8080 already in use"
  exit 0
fi

"$SERVER" &
SERVER_PID=$!
sleep 2

# Verify it's running
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
  echo "FAIL: server didn't start"
  exit 1
fi

# Verify it responds
HTTP_CODE=$(curl -s -o /dev/null -w '%{http_code}' http://localhost:8080/healthz 2>/dev/null || echo "000")
if [ "$HTTP_CODE" != "200" ]; then
  echo "FAIL: server not responding (got HTTP $HTTP_CODE)"
  kill "$SERVER_PID" 2>/dev/null || true
  exit 1
fi

# Send SIGTERM
kill -TERM "$SERVER_PID"

# Wait up to 10 seconds for clean exit
WAITED=0
while kill -0 "$SERVER_PID" 2>/dev/null; do
  if [ "$WAITED" -ge 10 ]; then
    echo "FAIL: server didn't exit within 10s after SIGTERM"
    kill -9 "$SERVER_PID" 2>/dev/null || true
    exit 1
  fi
  sleep 1
  WAITED=$((WAITED + 1))
done

# Check exit code
wait "$SERVER_PID" 2>/dev/null
EXIT_CODE=$?

if [ "$EXIT_CODE" -eq 0 ] || [ "$EXIT_CODE" -eq 143 ]; then
  echo "PASS: server shut down cleanly in ${WAITED}s (exit code $EXIT_CODE)"
  exit 0
else
  echo "FAIL: server exited with code $EXIT_CODE"
  exit 1
fi

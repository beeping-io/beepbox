#!/usr/bin/env bash
# Smoke test: build Docker image, run container, verify /healthz
set -euo pipefail

IMAGE_NAME="beepbox-server:smoke"
CONTAINER_NAME="beepbox-smoke-test"
PORT=18090

echo "=== Building image ==="
docker build -t "$IMAGE_NAME" .

echo "=== Starting container ==="
docker run -d --name "$CONTAINER_NAME" -p "${PORT}:8080" "$IMAGE_NAME"

cleanup() {
  echo "=== Cleaning up ==="
  docker stop "$CONTAINER_NAME" 2>/dev/null || true
  docker rm "$CONTAINER_NAME" 2>/dev/null || true
}
trap cleanup EXIT

# Wait for server to start
echo "=== Waiting for server ==="
for i in $(seq 1 15); do
  if curl -sf "http://localhost:${PORT}/healthz" >/dev/null 2>&1; then
    break
  fi
  if [ "$i" -eq 15 ]; then
    echo "FAIL: server didn't respond after 15s"
    docker logs "$CONTAINER_NAME"
    exit 1
  fi
  sleep 1
done

echo "=== Testing endpoints ==="

# /healthz
HTTP=$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:${PORT}/healthz")
[ "$HTTP" = "200" ] || { echo "FAIL: /healthz returned $HTTP"; exit 1; }
echo "  /healthz: $HTTP OK"

# /readyz
HTTP=$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:${PORT}/readyz")
[ "$HTTP" = "200" ] || { echo "FAIL: /readyz returned $HTTP"; exit 1; }
echo "  /readyz: $HTTP OK"

# /version
HTTP=$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:${PORT}/version")
[ "$HTTP" = "200" ] || { echo "FAIL: /version returned $HTTP"; exit 1; }
echo "  /version: $HTTP OK"

# /metrics
HTTP=$(curl -s -o /dev/null -w '%{http_code}' "http://localhost:${PORT}/metrics")
[ "$HTTP" = "200" ] || { echo "FAIL: /metrics returned $HTTP"; exit 1; }
echo "  /metrics: $HTTP OK"

# /v1/encode
HTTP=$(curl -s -o /dev/null -w '%{http_code}' -X POST \
  -H "Content-Type: application/json" \
  -d '{"key":"a1b2c","duration":2.3}' \
  "http://localhost:${PORT}/v1/encode")
[ "$HTTP" = "200" ] || { echo "FAIL: /v1/encode returned $HTTP"; exit 1; }
echo "  /v1/encode: $HTTP OK"

# Image size
SIZE=$(docker image inspect "$IMAGE_NAME" --format='{{.Size}}')
SIZE_MB=$((SIZE / 1024 / 1024))
echo ""
echo "=== Image size: ${SIZE_MB} MB ==="
echo "=== PASS: all smoke tests passed ==="

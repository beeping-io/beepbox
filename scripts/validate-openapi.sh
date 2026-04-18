#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SPEC="$SCRIPT_DIR/../docs/openapi.yaml"
VENV="/tmp/openapi-venv"

if [ ! -f "$VENV/bin/openapi-spec-validator" ]; then
  echo "Creating venv and installing openapi-spec-validator..."
  python3 -m venv "$VENV"
  "$VENV/bin/pip" install --quiet openapi-spec-validator
fi

echo "Validating $SPEC ..."
"$VENV/bin/openapi-spec-validator" "$SPEC"
echo "OpenAPI spec is valid."

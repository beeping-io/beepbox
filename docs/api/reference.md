# Beepbox API Reference

The Beepbox HTTP API is defined in [OpenAPI 3.1](openapi.yaml) as the
single source of truth for all endpoints, schemas, and examples.

## Interactive documentation

Open the spec in [Scalar](https://docs.scalar.com/swagger-editor) by
pasting the raw URL or uploading `docs/api/openapi.yaml`.

To browse locally with the server running:

```bash
# Start beepbox-server
./build/beepbox-server

# Open Scalar with the local spec
open "https://docs.scalar.com/swagger-editor?url=http://localhost:8080/openapi.yaml"
```

## Quick reference

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/healthz` | Liveness probe |
| `GET` | `/readyz` | Readiness probe (checks beeping-core) |
| `GET` | `/version` | Server + core version |
| `POST` | `/v1/encode` | Encode payload → WAV audio |
| `POST` | `/v1/decode` | Decode WAV audio → payload |

## Example: encode

```bash
curl -X POST http://localhost:8080/v1/encode \
  -H "Content-Type: application/json" \
  -d '{"key": "a1b2c", "mode": "inaudible"}' \
  -o output.wav
```

## Example: decode

```bash
curl -X POST http://localhost:8080/v1/decode \
  -H "Content-Type: audio/wav" \
  --data-binary @output.wav
```

Response:

```json
{
  "decoded": "a1b2c",
  "confidence": 0.95,
  "mode": 2
}
```

## Generated clients

Type-safe clients are auto-generated from `openapi.yaml` on every release
for Dart, Kotlin, Swift, TypeScript, and Python. Download them from
[GitHub Releases](https://github.com/beeping-io/beepbox/releases).

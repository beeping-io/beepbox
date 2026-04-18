# Observability Guide

beepbox-server provides distributed tracing via W3C Trace Context,
structured JSON logs with trace correlation, and Prometheus metrics.

## Distributed tracing

### How it works

Every request to `/v1/encode` and `/v1/decode` creates a **span** with:

- Unique `traceId` (32 hex chars) and `spanId` (16 hex chars)
- Timing (start → end duration in ms)
- Attributes: `http.method`, `http.url`, `http.status_code`, `beepbox.key_hash`
- Parent context propagated from incoming `traceparent` header

### W3C Trace Context propagation

Clients can pass a `traceparent` header ([W3C spec](https://www.w3.org/TR/trace-context/)):

```
traceparent: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
```

The server:
1. Extracts `traceId` and `parentSpanId` from the header
2. Creates a child span under that parent
3. Returns `traceresponse` and `X-Trace-Id` headers on the response

If no `traceparent` is provided, a new trace is generated automatically.

### Response headers

| Header | Description |
|--------|-------------|
| `traceresponse` | W3C traceparent for downstream propagation |
| `X-Trace-Id` | The trace ID (32 hex chars) for easy lookup |

### Cloud Trace integration (GCP)

On Cloud Run, spans are emitted as structured JSON logs to stdout.
Cloud Logging automatically correlates them with Cloud Trace using the
`logging.googleapis.com/spanId` and `logging.googleapis.com/trace_sampled`
fields in the JSON output.

No additional SDK or agent is required.

## Structured logs

Each span emits a JSON log line to stdout:

```json
{
  "severity": "INFO",
  "message": "encode",
  "logging.googleapis.com/spanId": "a1b2c3d4e5f67890",
  "logging.googleapis.com/trace_sampled": true,
  "span": {
    "name": "encode",
    "traceId": "4bf92f3577b34da6a3ce929d0e0e4736",
    "spanId": "a1b2c3d4e5f67890",
    "parentSpanId": "00f067aa0ba902b7",
    "durationMs": 15.23,
    "statusCode": 200,
    "attributes": {
      "http.method": "POST",
      "http.url": "/v1/encode",
      "http.status_code": "200",
      "beepbox.key_hash": "854e188e"
    }
  }
}
```

## Prometheus metrics

See [metrics.md](metrics.md) for the full metrics reference.

Available at `GET /metrics` in Prometheus exposition format.

## Environment variables

| Variable | Default | Description |
|----------|---------|-------------|
| `BEEPBOX_API_KEYS` | *(none)* | Comma-separated API keys (auth disabled if unset) |
| `BEEPBOX_RATE_LIMIT_RPM` | *(none)* | Requests per minute per key (disabled if unset) |
| `GCP_PROJECT_ID` | *(none)* | Used to format Cloud Trace resource names |

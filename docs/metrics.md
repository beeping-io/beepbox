# Beepbox Metrics Reference

The beepbox-server exposes metrics at `GET /metrics` in
[Prometheus exposition format](https://prometheus.io/docs/instrumenting/exposition_formats/).

## Metrics

### `beepbox_uptime_seconds` (gauge)

Time in seconds since the server started.

### `beepbox_requests_total` (counter)

Total HTTP requests to `/v1/*` endpoints, broken down by labels:

| Label | Description |
|-------|-------------|
| `key_hash` | FNV-1a hash (8 hex chars) of the API key for privacy |
| `endpoint` | Request path (`/v1/encode` or `/v1/decode`) |
| `status` | HTTP status code (`200`, `400`, `404`, `422`, `500`) |

### `beepbox_quota_consumed_total` (counter)

Total requests consumed per API key (all endpoints, all statuses).
Used by the developer portal to display usage dashboards.

| Label | Description |
|-------|-------------|
| `key_hash` | FNV-1a hash of the API key |

### `beepbox_request_duration_seconds` (histogram)

Request duration in seconds, broken down by key and endpoint.

| Label | Description |
|-------|-------------|
| `key_hash` | FNV-1a hash of the API key |
| `endpoint` | Request path |

Bucket boundaries: `0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0, +Inf`

## Example output

```
# HELP beepbox_uptime_seconds Time since server start.
# TYPE beepbox_uptime_seconds gauge
beepbox_uptime_seconds 3600

# HELP beepbox_requests_total Total HTTP requests by key, endpoint, status.
# TYPE beepbox_requests_total counter
beepbox_requests_total{key_hash="a1b2c3d4",endpoint="/v1/encode",status="200"} 142
beepbox_requests_total{key_hash="a1b2c3d4",endpoint="/v1/decode",status="200"} 87

# HELP beepbox_quota_consumed_total Total requests consumed per API key.
# TYPE beepbox_quota_consumed_total counter
beepbox_quota_consumed_total{key_hash="a1b2c3d4"} 229

# HELP beepbox_request_duration_seconds Request duration in seconds.
# TYPE beepbox_request_duration_seconds histogram
beepbox_request_duration_seconds_bucket{key_hash="a1b2c3d4",endpoint="/v1/encode",le="0.01"} 0
beepbox_request_duration_seconds_bucket{key_hash="a1b2c3d4",endpoint="/v1/encode",le="0.05"} 50
...
beepbox_request_duration_seconds_bucket{key_hash="a1b2c3d4",endpoint="/v1/encode",le="+Inf"} 142
beepbox_request_duration_seconds_sum{key_hash="a1b2c3d4",endpoint="/v1/encode"} 12.5
beepbox_request_duration_seconds_count{key_hash="a1b2c3d4",endpoint="/v1/encode"} 142
```

## Privacy

API keys are **never** exposed in metrics. The `key_hash` label uses a
truncated FNV-1a hash (8 hex characters) which is sufficient for
cardinality tracking but cannot be reversed to the original key.

## Scraping

Configure Prometheus to scrape `http://<host>:8080/metrics` at your
desired interval (recommended: 15s).

# Deploy Runbook

## Automated deploy (CI)

Deploys happen automatically on:
- **Release published** (tag `v*`) → builds, pushes to AR, deploys to Cloud Run, runs smoke tests
- **Manual dispatch** → same flow with custom image tag

The workflow (`.github/workflows/deploy.yml`) includes automatic rollback
if smoke tests fail after deploy.

## Manual deploy

### 1. Build and push image

```bash
# Build for amd64 (Cloud Run)
docker buildx build --platform linux/amd64 \
  -t europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:v0.1.0 \
  --push .
```

### 2. Deploy to Cloud Run

```bash
gcloud run deploy beepbox-server \
  --image europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:v0.1.0 \
  --region europe-west1 \
  --project beeping-platform-dev
```

### 3. Smoke test

```bash
./scripts/smoke.sh https://beepbox-server-ai7n45q5lq-ew.a.run.app
```

### 4. Load test (optional)

```bash
brew install k6  # if not installed
./scripts/load.sh https://beepbox-server-ai7n45q5lq-ew.a.run.app
```

## Rollback

### Quick rollback (traffic shift)

```bash
# List revisions
gcloud run revisions list \
  --service=beepbox-server \
  --region=europe-west1 \
  --format="table(name,active,creationTimestamp)"

# Route 100% traffic to previous revision
gcloud run services update-traffic beepbox-server \
  --region=europe-west1 \
  --to-revisions=beepbox-server-PREVIOUS=100
```

### Full rollback (redeploy old image)

```bash
gcloud run deploy beepbox-server \
  --image europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:PREVIOUS_TAG \
  --region europe-west1
```

## Verification

```bash
# Cloud Run direct
curl https://beepbox-server-ai7n45q5lq-ew.a.run.app/version
curl https://beepbox-server-ai7n45q5lq-ew.a.run.app/readyz
curl https://beepbox-server-ai7n45q5lq-ew.a.run.app/metrics

# Firebase Hosting (after DNS setup)
curl https://beeping-platform-dev-api.web.app/version
```

## Troubleshooting

| Symptom | Check |
|---------|-------|
| 503 Service Unavailable | `gcloud run services describe beepbox-server --region=europe-west1` — check revision status |
| Slow cold start | Check Cloud Run logs for startup time. Consider `min-instances=1` |
| Secret not found | `gcloud secrets versions list beepbox-api-keys` — verify latest version exists |
| Image not found | `gcloud artifacts docker images list europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox` |
| `/healthz` returns 404 | Known Cloud Run behavior — use `/readyz` for external health checks |

## URLs

| What | URL |
|------|-----|
| Cloud Run (direct) | `https://beepbox-server-ai7n45q5lq-ew.a.run.app` |
| Firebase Hosting | `https://beeping-platform-dev-api.web.app` |
| Custom domain (pending DNS) | `https://api.beeping.io` |
| Cloud Console | `https://console.cloud.google.com/run/detail/europe-west1/beepbox-server` |

# Infrastructure Guide

beepbox-server runs on **Google Cloud Run**, fronted by
**Firebase Hosting** at `api.beeping.io`.

## Architecture

```
Client → api.beeping.io (Firebase Hosting)
           ↓ rewrite **
         Cloud Run (beepbox-server)
           ↓ pulls image from
         Artifact Registry (europe-west1)
           ↓ reads secrets from
         Secret Manager (API keys, rate limit)
```

## Resources (Terraform)

All infrastructure is defined in `infra/`:

| Resource | Description |
|----------|-------------|
| `google_artifact_registry_repository` | Docker image repo `beepbox` |
| `google_cloud_run_v2_service` | `beepbox-server` (min=0, max=10) |
| `google_service_account` | `beepbox-server@project` (least-privilege) |
| `google_secret_manager_secret` x2 | API keys + rate limit RPM |
| `google_firebase_hosting_site` | Site `api` |
| `google_firebase_hosting_version` | Rewrite `**` → Cloud Run |

## Prerequisites

1. **gcloud CLI** authenticated: `gcloud auth application-default login`
2. **Terraform** >= 1.5: `brew install terraform`
3. **GCP project** with APIs enabled (already done in `beeping-platform-dev`)

## Deploy

```bash
cd infra

# First time
terraform init
terraform plan -out=plan.tfplan
terraform apply plan.tfplan

# Subsequent updates
terraform plan -out=plan.tfplan
terraform apply plan.tfplan
```

## Push a Docker image

```bash
# Authenticate Docker to Artifact Registry
gcloud auth configure-docker europe-west1-docker.pkg.dev

# Build and push
docker build -t europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:v0.0.1 .
docker push europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:v0.0.1

# Update Cloud Run to new image
gcloud run deploy beepbox-server \
  --image europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:v0.0.1 \
  --region europe-west1 \
  --project beeping-platform-dev
```

## Custom domain (api.beeping.io)

After Terraform apply, set up the custom domain:

1. In GoDaddy DNS, add a CNAME record:
   - Name: `api`
   - Value: `beeping-platform-dev-api.web.app`
   - TTL: 1 hour

2. In Firebase Console → Hosting → `api` site → Custom domains → Add `api.beeping.io`

3. Verify: `curl https://api.beeping.io/healthz`

## Secrets management

Update secrets via CLI:

```bash
# Update API keys
echo -n "bk_prod_key1,bk_prod_key2" | \
  gcloud secrets versions add beepbox-api-keys --data-file=- \
  --project=beeping-platform-dev

# Update rate limit
echo -n "120" | \
  gcloud secrets versions add beepbox-rate-limit-rpm --data-file=- \
  --project=beeping-platform-dev
```

Cloud Run picks up new secret versions on next cold start.

## Costs (dev environment)

| Resource | Estimated |
|----------|-----------|
| Cloud Run (min=0, scale to zero) | $0 (free tier) |
| Artifact Registry (~0.5 GB) | ~$0.10/mo |
| Secret Manager (2 secrets) | ~$0.12/mo |
| Firebase Hosting | $0 |
| **Total** | **~$0.22/mo** |

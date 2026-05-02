# CI/CD — beepbox-server

## Overview

`.github/workflows/deploy.yml` builds and deploys `beepbox-server` to
Google Cloud Run. Authentication uses **Workload Identity Federation
(WIF)** — no long-lived service account keys in GitHub.

## Triggers

| Trigger | Target | Image tag |
|---------|--------|-----------|
| `release: published` | **prod** | release tag (e.g. `v0.1.0`) |
| `workflow_dispatch` (manual) | `dev` *(default)* or `prod` | input `image_tag`, or `sha-<7chars>` if empty |

To deploy manually:

```bash
# Dev (most common during development)
gh workflow run deploy.yml -f target=dev

# Prod (override default)
gh workflow run deploy.yml -f target=prod -f image_tag=sha-96cf4a8
```

## Authentication (WIF)

GitHub Actions exchanges its OIDC token for short-lived GCP credentials
via Workload Identity Federation. No keys ever live in GitHub secrets;
only the **provider path** and **SA email** are stored.

```
┌─────────────────────┐                ┌─────────────────────────────┐
│ GitHub Actions      │  OIDC token    │ GCP Workload Identity Pool  │
│ workflow run        │ ─────────────► │ projects/<num>/.../pools/   │
└─────────────────────┘                │   github-actions            │
                                       │ Provider: github            │
                                       │ Condition:                  │
                                       │   assertion.repository ==   │
                                       │   'beeping-io/beepbox'      │
                                       └────────────┬────────────────┘
                                                    │ impersonate
                                                    ▼
                                       ┌─────────────────────────────┐
                                       │ SA: github-actions-deploy@  │
                                       │     {project}.iam.gservice  │
                                       │     account.com             │
                                       │ Roles:                      │
                                       │   - artifactregistry.writer │
                                       │   - run.admin               │
                                       │   - iam.serviceAccountUser  │
                                       └─────────────────────────────┘
```

The provider attribute condition (`assertion.repository == 'beeping-io/beepbox'`)
locks token exchange to **this repo only**. A token from any other
GitHub Actions workflow gets rejected.

## GitHub repo secrets

| Secret | Value | Set by |
|--------|-------|--------|
| `WIF_PROVIDER_DEV`  | `projects/113052191496/locations/global/workloadIdentityPools/github-actions/providers/github` | BEE-1803 |
| `WIF_SA_DEV`        | `github-actions-deploy@beeping-platform-dev.iam.gserviceaccount.com`  | BEE-1803 |
| `WIF_PROVIDER_PROD` | `projects/638946150252/locations/global/workloadIdentityPools/github-actions/providers/github` | BEE-1803 |
| `WIF_SA_PROD`       | `github-actions-deploy@beeping-platform-prod.iam.gserviceaccount.com` | BEE-1803 |

## Setup (re-run if needed)

The setup is idempotent. To reproduce on a fresh GCP project, for each
of `beeping-platform-{dev,prod}`:

```bash
PROJECT=beeping-platform-dev          # or prod
PROJECT_NUM=$(gcloud projects describe $PROJECT --format='value(projectNumber)')
POOL=github-actions
PROVIDER=github
SA_EMAIL="github-actions-deploy@${PROJECT}.iam.gserviceaccount.com"
REPO=beeping-io/beepbox

# 1. APIs
gcloud services enable sts.googleapis.com iam.googleapis.com \
  iamcredentials.googleapis.com cloudresourcemanager.googleapis.com \
  --project=$PROJECT

# 2. Pool + Provider
gcloud iam workload-identity-pools create $POOL \
  --project=$PROJECT --location=global \
  --display-name="GitHub Actions"

gcloud iam workload-identity-pools providers create-oidc $PROVIDER \
  --project=$PROJECT --location=global --workload-identity-pool=$POOL \
  --display-name="GitHub OIDC" \
  --issuer-uri="https://token.actions.githubusercontent.com" \
  --attribute-mapping="google.subject=assertion.sub,attribute.repository=assertion.repository,attribute.repository_owner=assertion.repository_owner,attribute.ref=assertion.ref" \
  --attribute-condition="assertion.repository == '${REPO}'"

# 3. SA + roles
gcloud iam service-accounts create github-actions-deploy --project=$PROJECT
sleep 10  # wait for SA propagation
for ROLE in roles/artifactregistry.writer roles/run.admin roles/iam.serviceAccountUser; do
  gcloud projects add-iam-policy-binding $PROJECT \
    --member="serviceAccount:$SA_EMAIL" --role=$ROLE --condition=None
done

# 4. Bind WIF principalSet → impersonate the SA
gcloud iam service-accounts add-iam-policy-binding $SA_EMAIL \
  --project=$PROJECT \
  --role=roles/iam.workloadIdentityUser \
  --member="principalSet://iam.googleapis.com/projects/${PROJECT_NUM}/locations/global/workloadIdentityPools/${POOL}/attribute.repository/${REPO}"

# 5. Set GitHub repo secrets
echo "projects/${PROJECT_NUM}/locations/global/workloadIdentityPools/${POOL}/providers/${PROVIDER}" | \
  gh secret set WIF_PROVIDER_DEV --repo beeping-io/beepbox  # or _PROD
echo "$SA_EMAIL" | gh secret set WIF_SA_DEV --repo beeping-io/beepbox    # or _PROD
```

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `Permission 'iam.serviceAccounts.getAccessToken' denied` | WIF binding missing or wrong principalSet path | Re-run step 4 above |
| `The caller does not have permission` after deploy | SA missing `run.admin` or `artifactregistry.writer` | Re-run step 3 |
| `Service account does not exist` | Race after SA creation; gcloud command ran too soon | Wait 10s, retry |
| Workflow fails at `Authenticate to GCP` | Repo secret missing | `gh secret list --repo beeping-io/beepbox` and verify all 4 `WIF_*` |
| Token rejected with `assertion.repository` mismatch | Trying to use these secrets from another repo (good — that's the security boundary) | Don't |

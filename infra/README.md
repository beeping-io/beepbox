# beepbox-server infrastructure

Multi-env Terraform layout. State lives remotely in
`gs://beeping-platform-dev-terraform/beepbox/{dev,prod}`.

## Layout

```
infra/
├── modules/                     reusable building blocks
│   ├── artifact-registry/       Docker repo (DOCKER format, keep last 10)
│   ├── cloud-run/               beepbox-server service (env vars + secret refs)
│   ├── firebase-hosting/        api.beeping.io rewrite to Cloud Run
│   ├── iam/                     least-privilege SA + project bindings
│   └── secrets/                 beepbox-api-keys + beepbox-rate-limit-rpm
└── envs/
    ├── dev/                     beeping-platform-dev
    └── prod/                    beeping-platform-prod
```

## Operating

### First-time setup on a new machine

```bash
cd infra/envs/dev   # or prod
terraform init
```

### Plan / apply

```bash
cd infra/envs/dev
terraform plan -out=plan.tfplan
terraform apply plan.tfplan
```

### Updating secret values

Secret resources are managed by Terraform but **values are not**. Update
out-of-band:

```bash
echo -n "bk_prod_real,bk_prod_real_2" | \
  gcloud secrets versions add beepbox-api-keys \
  --data-file=- --project=beeping-platform-prod
```

The `lifecycle { ignore_changes = [secret_data] }` block on the version
resource keeps Terraform from drifting the value back to the placeholder.

### Updating the Cloud Run image

CI does this via `gcloud run deploy ... --image ...`. Terraform ignores
image changes (`lifecycle { ignore_changes = [template[0].containers[0].image] }`)
so the next plan stays clean.

## Convention

- `region = europe-west1` on all resources.
- Service account `beepbox-server@{project}.iam.gserviceaccount.com` is
  the **only** identity beepbox-server runs as. Never the compute default.
- Secret values are placeholders on first apply; rotate via gcloud.

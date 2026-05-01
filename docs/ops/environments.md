# Environments — beepbox-server

Snapshot del estado real de los entornos GCP donde corre `beepbox-server`.
**Vive en git porque es code-adjacent.** Este documento se actualiza a
mano cuando cambia el bootstrap; no es source-of-truth automatizado.

> Última actualización: 2026-05-01 — tras BEE-1794 (CORS).

## Resumen ejecutivo

Hay **dos entornos GCP** distintos, mismo código pero **bootstrap muy
asimétrico**: dev está montado con Terraform y service account dedicado,
prod se desplegó a mano sin Terraform y usa el SA por defecto de Compute
Engine.

| | **dev** (`beeping-platform-dev`) | **prod** (`beeping-platform-prod`) |
|---|---|---|
| Cloud Run URL | `beepbox-server-ai7n45q5lq-ew.a.run.app` | `beepbox-server-jlqkyqxtca-ew.a.run.app` |
| Imagen actual | `sha-96cf4a8` (BEE-1794, 2026-04-29) | `:latest` (revision 00001, 2026-04-23) |
| Service Account | `beepbox-server@…dev` dedicado, least-privilege | ⚠️ `compute default` (`638946150252-compute@…`) |
| Secret Manager | `beepbox-api-keys` + `beepbox-rate-limit-rpm` | ⚠️ no existen — solo `RESEND_API_KEY` |
| `BEEPBOX_AUTH_ENDPOINT` | `…dev.cloudfunctions.net/validateApiKey` | `…prod.cloudfunctions.net/validateApiKey` |
| CORS | habilitado (localhost:3000 + dev Firebase) | desactivado |
| Artifact Registry | `beepbox/` repo en `europe-west1` | `beepbox/` repo en `europe-west1` |
| Firebase Hosting | site `api` con rewrite a Cloud Run | no aplicado |
| Custom domain | `api.beeping.io` → CNAME pendiente | n/a |

`/version` responde 200 en ambos. `api.beeping.io` no resuelve (DNS
pendiente en GoDaddy).

## Capas de credenciales

### Capa 1 — API keys de clientes (las que el caller manda en `Authorization`)

- Se **generan/validan/revocan en Cloud Functions**, no en este repo.
  Trío `generateApiKey` / `validateApiKey` / `revokeApiKey` desplegado
  en `europe-west1`, en dev y prod.
- Esas Functions **viven en otro repo** (probablemente
  `beeping-functions` — TODO confirmar y enlazar aquí).
- `beepbox-server` **no tiene store local de keys**: las valida
  remotamente vía `BEEPBOX_AUTH_ENDPOINT` (HttpKeyStore añadido en
  BEE-1687). El env var `BEEPBOX_API_KEYS` que aparece en dev es legacy
  CSV — fallback, pero el camino real es la Cloud Function.

### Capa 2 — credenciales de infra (runtime + deploys)

- **Runtime Cloud Run (dev)**: SA `beepbox-server@…dev` con
  `artifactregistry.reader` + `secretmanager.secretAccessor` +
  `logging.logWriter` + `cloudtrace.agent`. Definido en `infra/iam.tf`.
  **En prod no se aplicó** — usa el SA default de Compute Engine
  (demasiado amplio).
- **Deploys CI**: `.github/workflows/deploy.yml` espera secrets
  `WIF_PROVIDER` + `WIF_SA` (Workload Identity Federation). **Nunca se
  han creado** (ver `PENDING.md` → pending-001). Por eso BEE-1794 se
  desplegó a mano: `docker build` local + `gcloud run deploy`.
- **Secrets de servidor (dev)**: en Secret Manager
  (`beepbox-api-keys`, `beepbox-rate-limit-rpm`). Inyectados como env
  vars vía `value_source.secret_key_ref` con `version=latest`. Cloud
  Run los pickup en cold start.

## Terraform

`infra/` tiene todo (artifact-registry, cloud-run, firebase-hosting,
iam, secrets) **pero los defaults apuntan solo a `beeping-platform-dev`**.

- No hay workspace ni `tfvars` para prod → **prod nunca se aplicó con
  Terraform** (deploy manual).
- `terraform.tfstate` vive **local en `infra/`**. El backend GCS está
  comentado en `main.tf` — sin remote state, sin lock, sin colaboración
  segura.

## Flujo de deploy actual

### Pretendido (CI)

1. Release published o `workflow_dispatch` →
2. `.github/workflows/deploy.yml` autentica con WIF →
3. build + push a AR →
4. `google-github-actions/deploy-cloudrun` →
5. `scripts/smoke.sh` →
6. Rollback automático si smoke falla.

### Real hoy

Pipeline bloqueado porque WIF nunca se cableó. Cada release se hace a
mano:

```bash
docker buildx build --platform linux/amd64 \
  -t europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:vX.Y.Z \
  --push .

gcloud run deploy beepbox-server \
  --image europe-west1-docker.pkg.dev/beeping-platform-dev/beepbox/beepbox-server:vX.Y.Z \
  --region europe-west1 \
  --project beeping-platform-dev
```

(es lo que hicimos para BEE-1794, registrado en commit `19f13f2`.)

## Pendientes registrados (`docs/PENDING.md`)

- **pending-001** — Crear WIF Pool/Provider + SA con
  `artifactregistry.writer` + `run.admin` + `iam.serviceAccountUser`,
  bind al repo GitHub, exportar a `WIF_PROVIDER` / `WIF_SA`.
  Desbloquea `deploy.yml`.
- **pending-002** — `/healthz` desde fuera devuelve HTML 404 de Google
  Frontend (parece reservado por GFE). Cosmético — usamos `/readyz`
  para callers externos.

## Riesgos / inconsistencias detectadas

1. **Prod sin secrets de beepbox ni SA dedicado.** Si el server prod
   intenta leer `BEEPBOX_API_KEYS` o `BEEPBOX_RATE_LIMIT_RPM` desde env
   fallará silenciosamente o caerá a defaults. Confirmar que prod tira
   **solo** de `BEEPBOX_AUTH_ENDPOINT` y no necesita el CSV.
2. **Prod fuera de Terraform.** Sin IaC reproducible para prod. Drift
   garantizado entre lo que hay desplegado y lo que define `infra/`.
3. **WIF no cableado.** Todo deploy es manual desde la máquina del
   founder.
4. **Custom domain `api.beeping.io` no resuelve.** Falta CNAME en
   GoDaddy apuntando a `beeping-platform-dev-api.web.app`.
5. **Terraform state local.** `infra/terraform.tfstate` no está en GCS
   — riesgo de pérdida y bloqueo de colaboración.

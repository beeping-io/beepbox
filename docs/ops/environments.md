# Environments — beepbox-server

Snapshot del estado real de los entornos GCP donde corre `beepbox-server`.
**Vive en git porque es code-adjacent.** Este documento se actualiza a
mano cuando cambia el bootstrap; no es source-of-truth automatizado.

> Última actualización: 2026-05-02 — tras BEE-1799 (Terraform multi-env
> + GCS backend + prod parity).

## Resumen ejecutivo

Dos entornos GCP bajo la misma definición de Terraform multi-env. Estado
y bootstrap **idéntico** salvo por las URLs y la imagen actual (T2 las
alinea).

| | **dev** (`beeping-platform-dev`) | **prod** (`beeping-platform-prod`) |
|---|---|---|
| Cloud Run URL | `beepbox-server-ai7n45q5lq-ew.a.run.app` | `beepbox-server-jlqkyqxtca-ew.a.run.app` |
| Imagen actual | `sha-96cf4a8` (BEE-1794, 2026-04-29) | `:latest` (revision 00002, post-BEE-1799 — BEE-1800 la actualiza al sha de dev) |
| Service Account | `beepbox-server@…dev` dedicado, least-privilege | `beepbox-server@…prod` dedicado, least-privilege |
| Secret Manager | `beepbox-api-keys` + `beepbox-rate-limit-rpm` | `beepbox-api-keys` + `beepbox-rate-limit-rpm` (placeholders) |
| `BEEPBOX_AUTH_ENDPOINT` | `…dev.cloudfunctions.net/validateApiKey` | `…prod.cloudfunctions.net/validateApiKey` |
| CORS | `localhost:3000` + dev Firebase hosts | `https://beeping.io` (sin localhost, sin subdominios) |
| Artifact Registry | `beepbox/` repo en `europe-west1` | `beepbox/` repo en `europe-west1` |
| Firebase Hosting | site `beeping-platform-dev-api` | site `beeping-platform-prod-api` |
| Custom domain | pendiente DNS (T6: `api-dev.beeping.io`) | pendiente DNS (T6: `api.beeping.io`) |

`/version` responde 200 en ambos.

## Capas de credenciales

### Capa 1 — API keys de clientes (las que el caller manda en `Authorization`)

- Se generan/validan/revocan en **Cloud Functions** (TypeScript +
  Firebase Functions) que viven en `beeping-www/functions/`. Trío
  `generateApiKey` / `validateApiKey` / `revokeApiKey` desplegado en
  `europe-west1`, en dev y prod.
- `beepbox-server` no tiene store local de keys: las valida remotamente
  vía `BEEPBOX_AUTH_ENDPOINT` (HttpKeyStore añadido en BEE-1687).
- **TODO** (BEE-1801, T3): cerrar `allUsers` invoker en
  `generateApiKey` / `revokeApiKey` y exigir Firebase Auth.

### Capa 2 — credenciales de infra (runtime + deploys)

- **Runtime Cloud Run** (dev y prod): SA dedicado
  `beepbox-server@{project}.iam.gserviceaccount.com` con
  `artifactregistry.reader` + `secretmanager.secretAccessor` +
  `logging.logWriter` + `cloudtrace.agent`. Definido en
  `infra/modules/iam/`.
- **Secrets de servidor**: en Secret Manager (`beepbox-api-keys`,
  `beepbox-rate-limit-rpm`). Inyectados como env vars vía
  `value_source.secret_key_ref` con `version=latest`. Valores se
  rotan out-of-band con `gcloud secrets versions add` (Terraform
  ignora cambios al `secret_data` para no sobrescribir).
- **Deploys CI**: `.github/workflows/deploy.yml` espera secrets
  `WIF_PROVIDER` + `WIF_SA`. **Aún sin cablear** — BEE-1803 (T5) lo
  resuelve. Mientras, deploys son manuales desde la máquina del
  founder.

## Terraform

Layout multi-env con módulos compartidos:

```
infra/
├── modules/        artifact-registry, cloud-run, firebase-hosting, iam, secrets
└── envs/
    ├── dev/        backend gcs · prefix=beepbox/dev
    └── prod/       backend gcs · prefix=beepbox/prod
```

State remoto en `gs://beeping-platform-dev-terraform/beepbox/{env}`,
versionado activo.

```bash
cd infra/envs/{dev|prod}
terraform plan
terraform apply
```

Ver `infra/README.md` para operativa completa.

## Flujo de deploy actual

### Pretendido (CI · cuando T5 termine)

1. Release published o `workflow_dispatch -f target={dev|prod}` →
2. `.github/workflows/deploy.yml` autentica con WIF al proyecto target →
3. build + push a AR del proyecto target →
4. `deploy-cloudrun` →
5. `scripts/smoke.sh` →
6. Rollback automático si smoke falla.

### Real hoy (hasta T5)

```bash
# Build y push (target=dev | prod):
docker buildx build --platform linux/amd64 \
  -t europe-west1-docker.pkg.dev/beeping-platform-{env}/beepbox/beepbox-server:vX.Y.Z \
  --push .

gcloud run deploy beepbox-server \
  --image europe-west1-docker.pkg.dev/beeping-platform-{env}/beepbox/beepbox-server:vX.Y.Z \
  --region europe-west1 \
  --project beeping-platform-{env}
```

## Pendientes registrados (`docs/PENDING.md`)

- **pending-001** — WIF para `deploy.yml`. **Resuelto por BEE-1803**.
- **pending-002** — `/healthz` desde fuera devuelve HTML 404 de Google
  Frontend (parece reservado por GFE). Cosmético — usamos `/readyz`.
  **Documentado por BEE-1804**.

## Riesgos cerrados por BEE-1799

- ✅ Prod con SA dedicado + secrets en Secret Manager
- ✅ Prod managed by Terraform (sin drift)
- ✅ State remoto en GCS, versionado, sin riesgo de pérdida local

## Riesgos abiertos (siguientes tasks de Phase 2)

1. **Prod imagen desfasada** (build 2026-04-23, antes de BEE-1794) →
   BEE-1800 (T2) actualiza al sha actual.
2. **`generateApiKey` + `revokeApiKey` públicos (`allUsers`)** → BEE-1801
   (T3) cierra el agujero con Firebase Auth.
3. **Sin keys de testing en `.env.local`** → BEE-1802 (T4) genera keys
   y las guarda.
4. **WIF no cableado, deploys manuales** → BEE-1803 (T5).
5. **Custom domain DNS no resuelve** → BEE-1804 (T6).

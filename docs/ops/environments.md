# Environments — beepbox-server

Snapshot del estado real de los entornos GCP donde corre `beepbox-server`.
**Vive en git porque es code-adjacent.** Este documento se actualiza a
mano cuando cambia el bootstrap; no es source-of-truth automatizado.

> Última actualización: 2026-05-02 — tras BEE-1803 (WIF cableado
> para CI multi-env).

## Resumen ejecutivo

Dos entornos GCP bajo la misma definición de Terraform multi-env. Estado
y bootstrap **idéntico** salvo por las URLs y la imagen actual (T2 las
alinea).

| | **dev** (`beeping-platform-dev`) | **prod** (`beeping-platform-prod`) |
|---|---|---|
| Cloud Run URL | `beepbox-server-ai7n45q5lq-ew.a.run.app` | `beepbox-server-jlqkyqxtca-ew.a.run.app` |
| Imagen actual | `sha-96cf4a8` (BEE-1794, 2026-04-29) | `sha-96cf4a8` (revision 00003, copiado de dev AR via crane en BEE-1800) |
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
- **Deploys CI**: `.github/workflows/deploy.yml` autenticado vía WIF
  (BEE-1803), soporta `target=dev|prod`. Detalles en
  [`docs/ops/ci-cd.md`](ci-cd.md).

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

- **pending-002** — `/healthz` desde fuera devuelve HTML 404 de Google
  Frontend (parece reservado por GFE). Cosmético — usamos `/readyz`.
  **Documentado por BEE-1804**.

## Riesgos cerrados por BEE-1799

- ✅ Prod con SA dedicado + secrets en Secret Manager
- ✅ Prod managed by Terraform (sin drift)
- ✅ State remoto en GCS, versionado, sin riesgo de pérdida local

## Riesgos cerrados por BEE-1800

- ✅ Prod corre el mismo binario que dev (`sha-96cf4a8`)
- ✅ CORS verificado: `https://beeping.io` permitido en prod;
  `localhost`, `www.beeping.io`, `app.beeping.io` rebotan (sin
  `Access-Control-Allow-Origin`)

## Riesgos abiertos (siguientes tasks de Phase 2)

1. **`generateApiKey` + `revokeApiKey` públicos (`allUsers`)** → BEE-1801
   (T3) cierra el agujero con Firebase Auth.
2. **Sin keys de testing en `.env.local`** → BEE-1802 (T4) genera keys
   y las guarda.
3. **WIF no cableado, deploys manuales** → BEE-1803 (T5).
4. **Custom domain DNS no resuelve** → BEE-1804 (T6).

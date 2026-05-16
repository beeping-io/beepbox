# ⏳ Pending

Captura **trabajo conocido pero aún sin fecha** — el "lo haremos algún día pero
no ahora".

🎯 **Aquí entra**: deuda detectada, follow-ups de incidentes, feedback accionable,
"esto hay que hacerlo pero no hemos decidido cuándo".
🚫 **Aquí NO entra**: trabajo ya agendado a un milestone (eso va a Linear).

🪄 **Promoción**: ponerle un milestone a un pending lo convierte en task Linear
`BEE-XXXX` y se elimina automáticamente de este fichero.

---

## 📋 Cómo añadir un pending

Usa el skill `/pending` (recomendado). O copia este bloque al final del fichero:

```markdown
### ⏳ pending-NNN — [Título corto]

- 📅 **Fecha añadida**: YYYY-MM-DD
- 🏷️ **Tipo**: feat | fix | docs | refactor | chore | infra | security | test
- 🧭 **Trigger**: por qué se añadió (incidente, feedback, deuda)
- ⚙️ **Acción requerida**: qué hay que hacer concretamente
- 🚧 **Bloqueado por**: (si aplica) algo o alguien que retrasa
- 🚦 **Estado**: 🆕 Nuevo
```

### 🏷️ Tipos disponibles

| Tipo | Cuándo usarlo |
|------|---------------|
| `feat` | Funcionalidad nueva |
| `fix` | Bug fix |
| `docs` | Solo documentación |
| `refactor` | Refactor sin cambio de comportamiento |
| `chore` | Mantenimiento, deps, config |
| `infra` | Infraestructura, CI/CD |
| `security` | Cuestiones de seguridad |
| `test` | Solo tests |

### 🚦 Estados posibles

| Iconito | Estado | Significado |
|---------|--------|-------------|
| 🆕 | Nuevo | Recién capturado, sin triage |
| 🔍 | En triage | Decidiendo prioridad / scope |
| 📋 | Promovido | Ya es task Linear (`BEE-XXXX`) — debería haberse eliminado de aquí |
| 🚧 | Bloqueado | Esperando algo externo (especificar) |
| ❌ | No procede | Decidido no avanzar (apuntar el porqué) |

---

## 🗂️ Pendientes registrados

<!-- pending-002 (/healthz GFE intercept) cerrado por BEE-1804:
     documentado en docs/ops/deploy-runbook.md como
     comportamiento esperado de Cloud Run, no bug. -->

### ⏳ pending-003 — BEE-1802 test API keys no validan en dev ni prod

* 📅 **Fecha añadida**: 2026-05-16
* 🏷️ **Tipo**: fix
* 🧭 **Trigger**: smoke E2E de v0.1.0 deploy en dev + prod. Tanto `BEEPBOX_TEST_KEY_DEV=bk_PkvVhXmkqJncUaoCf-kPvQ` como `BEEPBOX_TEST_KEY_PROD=[REDACTED-BEEPBOX-API-KEY]` (minteadas en BEE-1802 para SDK E2E suites) devuelven `HTTP 403 {"error":"Invalid API key"}` contra `/v1/encode` y `/v1/decode` en sus respectivos environments. El binary procesa el request y rechaza vía `requireAuth` → el problema es validator-side, no servidor.
* ⚙️ **Acción requerida**: investigar por qué el HttpKeyStore (Cloud Function `validateApiKey`) rechaza estos hashes:
  1. Comprobar Firestore collection `apiKeys` (o donde guarde los hashes) en `beeping-platform-dev` y `beeping-platform-prod` — ver si los docs existen, si tienen estado `revoked`, o si el hash almacenado coincide con el SHA256 del key claro
  2. Hit directo al Cloud Function `validateApiKey` con el key y leer el response — confirma si el rechazo viene de la function o del cache local
  3. Si están perdidos: re-mintear vía `generateApiKey` (BEE-1801 requiere Firebase Auth ahora) y actualizar `.env.local` + GCP Secret Manager sync
* 🚧 **Bloqueado por**: nada (independiente del refactor BEE-2239)
* 🚦 **Estado**: 🆕 Nuevo

### ⏳ pending-004 — Docker Build & Push (GHCR) workflow roto + commit:unknown en binarios

* 📅 **Fecha añadida**: 2026-05-16
* 🏷️ **Tipo**: fix
* 🧭 **Trigger**: durante el deploy de v0.1.0 se detectaron 2 fallos del pipeline GHCR:
  1. **Trivy version pin inválido**: `.github/workflows/docker-build.yml:89` usa `aquasecurity/trivy-action@0.28.0` pero la versión `0.28.0` no existe (disponibles: `v0.29.0+`). Todo run del workflow Docker Build & Push ha fallado en el step `Security scan & sign` desde el commit BEE-1674 release-please.
  2. **`/version` reporta `commit:"unknown"`**: el `BEEPBOX_GIT_SHA` macro definido en CMakeLists.txt no se propaga al binary cuando se construye dentro del Docker container (probablemente `git` no disponible en build stage o el contexto no incluye `.git/`).
* ⚙️ **Acción requerida**:
  * Bump trivy-action pin a una versión existente (e.g. `v0.29.0` o un major estable)
  * Pasar `BEEPBOX_GIT_SHA` como `--build-arg` en `docker build` y exponerlo en Dockerfile via `ARG`/`ENV` → el binary lo recoge desde env var en lugar de macro embebido
  * Nota: el `Deploy to Cloud Run` workflow no depende de GHCR (construye su propia imagen para Artifact Registry), por eso los deploys SÍ funcionaron en dev+prod incluso con GHCR roto
* 🚧 **Bloqueado por**: nada
* 🚦 **Estado**: 🆕 Nuevo

### ⏳ pending-005 — Deploy a prod no auto-dispara en release:published (release-please + GITHUB_TOKEN)

* 📅 **Fecha añadida**: 2026-05-16
* 🏷️ **Tipo**: ci
* 🧭 **Trigger**: PR #10 mergeada → release-please creó tag `v0.1.0` + GitHub Release `published`. El workflow `Deploy to Cloud Run` tiene `on: release: [published]` pero **no se disparó**. Causa documentada: cuando release-please-action usa `GITHUB_TOKEN` para publicar la release, GitHub bloquea cascadas a otros workflows por defecto (anti-loop). Requirió `workflow_dispatch` manual para llegar a prod.
* ⚙️ **Acción requerida** (opciones):
  1. Usar un PAT (`secrets.RELEASE_PLEASE_PAT`) con permisos en lugar de `GITHUB_TOKEN` en `.github/workflows/release-please.yml` → la release event sí dispara workflows downstream. Costo: gestionar PAT con expiración.
  2. Añadir `repository_dispatch` desde release-please job a un workflow custom que llame al deploy. Más config pero sin PAT.
  3. Dejar deploy manual como política (cada release implica un `workflow_dispatch` consciente) — más seguro pero requiere checklist explícito.
* 🚧 **Bloqueado por**: nada
* 🚦 **Estado**: 🆕 Nuevo

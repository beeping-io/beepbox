# ⏳ Pending

Captura **trabajo conocido pero aún sin fecha** — el "lo haremos algún día pero
no ahora".

🎯 **Aquí entra**: deuda detectada, follow-ups de incidentes, feedback accionable,
"esto hay que hacerlo pero no hemos decidido cuándo".
🚫 **Aquí NO entra**: trabajo ya agendado a un milestone (eso va a GitHub Projects).

🪄 **Promoción**: ponerle un milestone a un pending lo convierte en issue en GitHub Projects
(`task-#<id>`) y se elimina automáticamente de este fichero.

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
| 📋 | Promovido | Ya es issue en GitHub Projects (`#<id>`) — debería haberse eliminado de aquí |
| 🚧 | Bloqueado | Esperando algo externo (especificar) |
| ❌ | No procede | Decidido no avanzar (apuntar el porqué) |

---

## 🗂️ Pendientes registrados

<!-- pending-002 (/healthz GFE intercept) cerrado por BEE-1804:
     documentado en docs/ops/deploy-runbook.md como
     comportamiento esperado de Cloud Run, no bug. -->
<!-- pending-003 (test API keys en dev/prod) cerrado por BEEP-93 (#93):
     hashes SHA-256 sincronizados en validateApiKey / Firestore para
     BEEPBOX_TEST_KEY_DEV y BEEPBOX_TEST_KEY_PROD. -->

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

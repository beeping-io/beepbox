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

### ⏳ pending-001 — Wire WIF for `deploy.yml` workflow

- 📅 **Fecha añadida**: 2026-04-29
- 🏷️ **Tipo**: infra
- 🧭 **Trigger**: durante el deploy de BEE-1794 (CORS) descubrimos
  que `deploy.yml` falla en el step `Authenticate to GCP` porque
  los secrets `WIF_PROVIDER` y `WIF_SA` no están configurados en
  el repo `beepbox`. El workflow se quedó a medio bootstrappear —
  probablemente desde BEE-1691 (CI) cuando se creó la pipeline.
  Workaround usado para BEE-1794: build local + `gcloud run deploy`
  desde la máquina del founder.
- ⚙️ **Acción requerida**: crear el Workload Identity Pool +
  Provider en GCP (`beeping-platform-dev` project), un Service
  Account con permisos `roles/artifactregistry.writer` +
  `roles/run.admin` + `roles/iam.serviceAccountUser`, bind GitHub
  org/repo via principalSet, exportar provider name + SA email a
  GitHub Actions secrets `WIF_PROVIDER` + `WIF_SA`. Documentar el
  setup en `docs/deploy-runbook.md`.
- 🚧 **Bloqueado por**: nada bloquea — siguiente release a Cloud
  Run forzará el setup. Mientras, deploys siguen siendo manuales
  via `gcloud` + Docker local.
- 🚦 **Estado**: 🆕 Nuevo

### ⏳ pending-002 — `/healthz` interceptado por Google Frontend

- 📅 **Fecha añadida**: 2026-04-29
- 🏷️ **Tipo**: infra
- 🧭 **Trigger**: durante la verificación post-deploy de BEE-1794
  observamos que `GET /healthz` desde fuera devuelve un HTML 404
  de Google Frontend (no llega a nuestro server), aunque el mismo
  endpoint funciona para los probes internos del Cloud Run y
  externamente `/readyz` y `/version` responden correctamente
  desde nuestro app. Probable: Cloud Run / GFE intercepta
  `/healthz` para health-check interno y nunca lo enruta al
  contenedor desde tráfico externo, mientras que internamente
  para probes sí funciona. Cosmético (`/readyz` cubre el mismo
  uso para callers externos), pero confunde si alguien debugea el
  servicio con curl.
- ⚙️ **Acción requerida**: investigar si Cloud Run reserva
  `/healthz`. Si sí, documentar en `docs/deploy-runbook.md` que
  los callers externos deben usar `/readyz`. Si no, abrir issue
  con GCP support.
- 🚧 **Bloqueado por**: nada — bug cosmético, no afecta
  funcionalidad.
- 🚦 **Estado**: 🆕 Nuevo

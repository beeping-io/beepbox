# beepbox docs

Índice de la documentación del repo. Organizada por intención de uso.

## API — qué expone el server

- [`api/reference.md`](api/reference.md) — guía rápida de endpoints + ejemplos.
- [`api/openapi.yaml`](api/openapi.yaml) — OpenAPI 3.1, source of truth.
  El CI lo valida y genera clientes (Dart, Kotlin, Swift, TS, Python).

## Ops — cómo se opera el server

- [`ops/infrastructure.md`](ops/infrastructure.md) — arquitectura GCP +
  Terraform "how to" (Cloud Run, Artifact Registry, Secret Manager,
  Firebase Hosting).
- [`ops/environments.md`](ops/environments.md) — snapshot real
  dev↔prod (URLs, SAs, secrets, CORS, riesgos detectados).
- [`ops/deploy-runbook.md`](ops/deploy-runbook.md) — pasos de deploy
  manual + rollback + troubleshooting.
- [`ops/supply-chain.md`](ops/supply-chain.md) — firma de imágenes,
  SBOM, attestations.
- [`ops/ci-cd.md`](ops/ci-cd.md) — Workload Identity Federation +
  multi-env deploys (`gh workflow run deploy.yml -f target=...`).

## Observability — qué se ve en runtime

- [`observability/metrics.md`](observability/metrics.md) — métricas
  Prometheus que expone `/metrics`.
- [`observability/tracing.md`](observability/tracing.md) — W3C Trace
  Context, OpenTelemetry, propagación.

## Captura de trabajo no formal

- [`IDEAS.md`](IDEAS.md) — brainstorms / "podríamos hacer X algún día".
- [`PENDING.md`](PENDING.md) — TODOs conocidos sin task Linear todavía.

> Estos dos viven en raíz por convención global del ecosistema (las
> skills `/ideas` y `/pending` los leen de ahí). No mover.

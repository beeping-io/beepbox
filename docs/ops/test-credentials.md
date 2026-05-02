# Test credentials for SDK E2E suites

`BEEPBOX_TEST_KEY_DEV` and `BEEPBOX_TEST_KEY_PROD` exist as test API
keys for the SDK E2E test suites (`beeping-{android,ios,flutter,web,
react-native,node,python,cli}`). Each key authenticates to the matching
beepbox-server environment.

## Where they live

- **Local**: `beepbox/.env.local` (gitignored)
- **GCP backup**: `gs://beeping-platform-dev/secrets/beepbox-env-local-dev`
  (versioned secret, auto-detected by `~/.claude/scripts/restore-env-from-gcp.sh`)

To restore on a new machine:

```bash
cd beepbox
~/.claude/scripts/restore-env-from-gcp.sh beepbox-env-local-dev beeping-platform-dev
```

## Provenance

Both keys are owned by the synthetic UID `beepbox-sdk-test` (no real
Firebase user). They live in the `apiKeys/{sha256(key)}` Firestore
collection of the corresponding GCP project, indistinguishable at the
`validateApiKey` Cloud Function boundary from keys minted by real
logged-in users via the production flow.

## Generating new test keys

If the existing keys are compromised or you need to rotate:

```bash
# Mint a fresh key for the target environment
DEV_KEY=$(./scripts/mint-test-key.py beeping-platform-dev beepbox-sdk-test \
            "SDK E2E test key (dev)")

# Replace in .env.local
sed -i '' "s|^BEEPBOX_TEST_KEY_DEV=.*|BEEPBOX_TEST_KEY_DEV=${DEV_KEY}|" .env.local

# Sync
~/.claude/scripts/sync-env-to-gcp.sh beepbox-env-local-dev beeping-platform-dev
```

Same flow for prod (project `beeping-platform-prod`,
`BEEPBOX_TEST_KEY_PROD`).

## Revoking a test key

Edit the Firestore doc and flip `revoked: true`:

```bash
KEY_HASH=$(echo -n "<the-bk_xxx-key>" | shasum -a 256 | awk '{print $1}')
gcloud firestore documents update --project=beeping-platform-{dev|prod} \
  apiKeys/$KEY_HASH revoked=true
```

(Or call `revokeApiKey` Callable Function with the user's Firebase
session — but for a synthetic test owner that's not logged-in anywhere,
direct Firestore update is the path.)

## Rotation policy

No fixed schedule. Rotate when:

- A `.env.local` gets exposed (e.g. screenshot, accidental commit, git
  history leak) → revoke immediately, mint new, update GCP secret.
- A test machine is decommissioned and its `.env.local` is unrecoverable
  (rotate so it's clear which key is current).
- Annually as hygiene.

## What these keys are NOT

- Not for **production** clients. Public consumers must mint their own
  keys via the (future) admin UI in `beeping-www` — see BEE-1801 for the
  auth model.
- Not for **load testing**. The rate limit on each env applies to all
  consumers of a single key — load tests against prod with this key
  would throttle real callers.
- Not embeddable in **public source code**. They live in `.env.local`
  (gitignored) for a reason.

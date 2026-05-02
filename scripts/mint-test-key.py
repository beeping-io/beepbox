#!/usr/bin/env python3
"""
Mint a beepbox test API key for SDK E2E suites.

Replicates the logic of `beeping-www/functions/src/generateApiKey.ts`
`generateApiKeyCore()` using Firestore REST directly — bypasses the
Callable Function (which would require a Firebase ID token), but writes
to the same `apiKeys/{sha256}` collection so beepbox-server's HttpKeyStore
validates it identically.

Auth: gcloud Application Default Credentials (the user must be logged in
with a principal that has `datastore.user` on the target Firebase project).

Usage:
    ./scripts/mint-test-key.py <project-id> [<owner-uid> [<label>]]

Example:
    ./scripts/mint-test-key.py beeping-platform-dev beepbox-sdk-test \\
        "BEE-1802 SDK E2E test key (dev)"

Output (stdout): the raw key. Capture and store securely; the raw form
is only shown ONCE (after that only `keyPrefix` is retrievable from the
Firestore doc, matching the production-app guarantee).
"""
import base64
import hashlib
import json
import secrets
import subprocess
import sys
import urllib.request
from datetime import datetime, timezone


def mint(project: str, owner_uid: str, label: str) -> str:
    raw_bytes = secrets.token_bytes(16)
    raw_key = "bk_" + base64.urlsafe_b64encode(raw_bytes).decode().rstrip("=")
    key_hash = hashlib.sha256(raw_key.encode()).hexdigest()
    key_prefix = raw_key[:7] + "…"

    token = subprocess.check_output(
        ["gcloud", "auth", "print-access-token"], text=True
    ).strip()

    url = (
        f"https://firestore.googleapis.com/v1/projects/{project}"
        f"/databases/(default)/documents/apiKeys/{key_hash}"
    )
    body = {
        "fields": {
            "ownerUid": {"stringValue": owner_uid},
            "label": {"stringValue": label} if label else {"nullValue": None},
            "keyPrefix": {"stringValue": key_prefix},
            "createdAt": {
                "timestampValue": datetime.now(timezone.utc)
                .isoformat(timespec="seconds")
                .replace("+00:00", "Z")
            },
            "lastUsed": {"nullValue": None},
            "revoked": {"booleanValue": False},
        }
    }
    req = urllib.request.Request(
        url,
        data=json.dumps(body).encode(),
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
        },
        method="PATCH",
    )
    try:
        with urllib.request.urlopen(req) as resp:
            resp.read()
    except urllib.error.HTTPError as e:
        msg = e.read().decode(errors="replace")
        raise SystemExit(
            f"Firestore write failed ({e.code}): {msg}\n"
            f"Verify ADC has datastore.user on {project}."
        )
    print(f"[mint-test-key] key_hash={key_hash} keyPrefix={key_prefix}", file=sys.stderr)
    return raw_key


if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    project = sys.argv[1]
    owner_uid = sys.argv[2] if len(sys.argv) >= 3 else "beepbox-sdk-test"
    label = sys.argv[3] if len(sys.argv) >= 4 else f"SDK E2E test key ({project})"
    raw = mint(project, owner_uid, label)
    print(raw)

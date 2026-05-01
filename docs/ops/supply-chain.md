# Supply Chain Security

Every beepbox-server Docker image published to GHCR is:

1. **Scanned** for vulnerabilities with [Trivy](https://trivy.dev/)
2. **Signed** with [cosign](https://docs.sigstore.dev/cosign/overview/)
   (keyless, Sigstore OIDC)
3. **Accompanied by an SBOM** in
   [CycloneDX](https://cyclonedx.org/) format

## Verify a signature

```bash
cosign verify \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com \
  --certificate-identity-regexp "github.com/beeping-io/beepbox" \
  ghcr.io/beeping-io/beepbox:latest
```

## Download and inspect the SBOM

```bash
cosign download attestation \
  --predicate-type https://cyclonedx.org/bom \
  ghcr.io/beeping-io/beepbox:latest \
  | jq -r '.payload' | base64 -d | jq .
```

## Scan locally

```bash
trivy image ghcr.io/beeping-io/beepbox:latest
```

## CI pipeline

The security pipeline runs in `.github/workflows/docker-build.yml`:

| Step | Tool | What it does |
|------|------|-------------|
| Trivy scan | `aquasecurity/trivy-action` | Fails CI on CRITICAL/HIGH vulns, uploads SARIF to GitHub Security tab |
| SBOM | `anchore/sbom-action` | Generates CycloneDX JSON, uploaded as artifact |
| Cosign sign | `sigstore/cosign-installer` | Keyless signing via GitHub OIDC identity |
| Cosign attest | `cosign attest` | Attaches SBOM as in-toto attestation to the image |

## Policy

- **CRITICAL** vulnerabilities block the build
- **HIGH** vulnerabilities block the build
- **MEDIUM** and below are informational (visible in GitHub Security tab)
- All images must be signed before deployment to Cloud Run

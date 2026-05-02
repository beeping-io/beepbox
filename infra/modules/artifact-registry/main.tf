resource "google_artifact_registry_repository" "beepbox" {
  repository_id = "beepbox"
  location      = var.region
  format        = "DOCKER"
  description   = "beepbox-server Docker images"

  cleanup_policies {
    id     = "keep-last-10"
    action = "KEEP"
    most_recent_versions {
      keep_count = 10
    }
  }
}

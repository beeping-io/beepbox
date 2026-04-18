# API keys for beepbox-server authentication
resource "google_secret_manager_secret" "api_keys" {
  secret_id = "beepbox-api-keys"

  replication {
    auto {}
  }
}

# Initial secret version (placeholder — update via console or CLI)
resource "google_secret_manager_secret_version" "api_keys_v1" {
  secret      = google_secret_manager_secret.api_keys.id
  secret_data = "bk_dev_placeholder"
}

# Rate limit configuration
resource "google_secret_manager_secret" "rate_limit" {
  secret_id = "beepbox-rate-limit-rpm"

  replication {
    auto {}
  }
}

resource "google_secret_manager_secret_version" "rate_limit_v1" {
  secret      = google_secret_manager_secret.rate_limit.id
  secret_data = "60"
}

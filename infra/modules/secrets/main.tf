resource "google_secret_manager_secret" "api_keys" {
  secret_id = "beepbox-api-keys"

  replication {
    auto {}
  }
}

resource "google_secret_manager_secret_version" "api_keys_v1" {
  secret      = google_secret_manager_secret.api_keys.id
  secret_data = var.api_keys_placeholder

  lifecycle {
    ignore_changes = [secret_data]
  }
}

resource "google_secret_manager_secret" "rate_limit" {
  secret_id = "beepbox-rate-limit-rpm"

  replication {
    auto {}
  }
}

resource "google_secret_manager_secret_version" "rate_limit_v1" {
  secret      = google_secret_manager_secret.rate_limit.id
  secret_data = var.rate_limit_placeholder

  lifecycle {
    ignore_changes = [secret_data]
  }
}

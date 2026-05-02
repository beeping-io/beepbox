output "api_keys_secret_id" {
  value = google_secret_manager_secret.api_keys.secret_id
}

output "rate_limit_secret_id" {
  value = google_secret_manager_secret.rate_limit.secret_id
}

output "service_account_email" {
  description = "beepbox-server Cloud Run service account email"
  value       = google_service_account.beepbox_server.email
}

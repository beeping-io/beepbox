output "cloud_run_url" {
  description = "Direct Cloud Run URL (auto-generated)"
  value       = google_cloud_run_v2_service.beepbox.uri
}

output "firebase_hosting_url" {
  description = "Firebase Hosting URL (before custom domain)"
  value       = "https://${google_firebase_hosting_site.api.site_id}.web.app"
}

output "artifact_registry_repo" {
  description = "Full Artifact Registry image path prefix"
  value       = "${var.region}-docker.pkg.dev/${var.project_id}/${google_artifact_registry_repository.beepbox.repository_id}"
}

output "service_account_email" {
  description = "beepbox-server service account email"
  value       = google_service_account.beepbox_server.email
}

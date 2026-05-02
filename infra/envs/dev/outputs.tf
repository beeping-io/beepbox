output "cloud_run_url" {
  description = "Direct Cloud Run URL"
  value       = module.cloud_run.service_uri
}

output "firebase_hosting_url" {
  description = "Firebase Hosting URL (before custom domain)"
  value       = module.firebase_hosting.site_url
}

output "artifact_registry_repo" {
  description = "Full Artifact Registry path prefix"
  value       = "${var.region}-docker.pkg.dev/${var.project_id}/${module.artifact_registry.repository_id}"
}

output "service_account_email" {
  description = "beepbox-server runtime SA"
  value       = module.iam.service_account_email
}

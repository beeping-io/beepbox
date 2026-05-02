output "repository_id" {
  description = "Artifact Registry repository ID"
  value       = google_artifact_registry_repository.beepbox.repository_id
}

output "repository_name" {
  description = "Full repository resource name (used by IAM bindings)"
  value       = google_artifact_registry_repository.beepbox.name
}

output "service_name" {
  description = "Cloud Run service name (used by Firebase Hosting rewrite)"
  value       = google_cloud_run_v2_service.beepbox.name
}

output "service_uri" {
  description = "Direct Cloud Run URL (auto-generated)"
  value       = google_cloud_run_v2_service.beepbox.uri
}

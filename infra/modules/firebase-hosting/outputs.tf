output "site_id" {
  description = "Firebase Hosting site ID"
  value       = google_firebase_hosting_site.api.site_id
}

output "site_url" {
  description = "Firebase Hosting URL (before custom domain mapping)"
  value       = "https://${google_firebase_hosting_site.api.site_id}.web.app"
}

output "site_id" {
  description = "Firebase Hosting site ID"
  value       = google_firebase_hosting_site.api.site_id
}

output "site_url" {
  description = "Firebase Hosting URL (before custom domain mapping)"
  value       = "https://${google_firebase_hosting_site.api.site_id}.web.app"
}

output "custom_domain_dns" {
  description = "DNS records that must be added to the registrar to verify the custom domain. Empty if no custom domain configured."
  value       = var.custom_domain == "" ? null : try(google_firebase_hosting_custom_domain.api[0].required_dns_updates, null)
}

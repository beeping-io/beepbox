resource "google_firebase_hosting_site" "api" {
  provider = google-beta
  project  = var.project_id
  site_id  = "${var.project_id}-api"
}

resource "google_firebase_hosting_version" "api" {
  provider = google-beta
  site_id  = google_firebase_hosting_site.api.site_id

  config {
    rewrites {
      glob = "**"
      run {
        service_id = var.cloud_run_service_name
        region     = var.region
      }
    }
  }
}

resource "google_firebase_hosting_release" "api" {
  provider     = google-beta
  site_id      = google_firebase_hosting_site.api.site_id
  version_name = google_firebase_hosting_version.api.name
}

# Optional custom domain mapping (e.g. api.beeping.io). Created in
# `NEEDS_DNS` state until the corresponding records exist; after DNS
# resolves Firebase auto-issues an SSL cert. wait_dns_verification=false
# so terraform apply doesn't block on user-driven DNS work.
resource "google_firebase_hosting_custom_domain" "api" {
  count    = var.custom_domain != "" ? 1 : 0
  provider = google-beta
  project  = var.project_id
  site_id  = google_firebase_hosting_site.api.site_id

  custom_domain         = var.custom_domain
  cert_preference       = "GROUPED"
  wait_dns_verification = false
}

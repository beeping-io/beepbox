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

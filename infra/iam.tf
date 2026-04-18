# Service account for beepbox-server Cloud Run
resource "google_service_account" "beepbox_server" {
  account_id   = "beepbox-server"
  display_name = "beepbox-server Cloud Run SA"
  description  = "Least-privilege SA for beepbox-server"
}

# Allow Cloud Run to pull images from Artifact Registry
resource "google_artifact_registry_repository_iam_member" "beepbox_reader" {
  repository = google_artifact_registry_repository.beepbox.name
  location   = var.region
  role       = "roles/artifactregistry.reader"
  member     = "serviceAccount:${google_service_account.beepbox_server.email}"
}

# Allow Cloud Run to access secrets
resource "google_project_iam_member" "secret_accessor" {
  project = var.project_id
  role    = "roles/secretmanager.secretAccessor"
  member  = "serviceAccount:${google_service_account.beepbox_server.email}"
}

# Allow Cloud Run to write logs
resource "google_project_iam_member" "log_writer" {
  project = var.project_id
  role    = "roles/logging.logWriter"
  member  = "serviceAccount:${google_service_account.beepbox_server.email}"
}

# Allow Cloud Run to export traces
resource "google_project_iam_member" "trace_writer" {
  project = var.project_id
  role    = "roles/cloudtrace.agent"
  member  = "serviceAccount:${google_service_account.beepbox_server.email}"
}

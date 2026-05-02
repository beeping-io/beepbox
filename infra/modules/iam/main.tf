resource "google_service_account" "beepbox_server" {
  account_id   = "beepbox-server"
  display_name = "beepbox-server Cloud Run SA"
  description  = "Least-privilege SA for beepbox-server"
}

resource "google_artifact_registry_repository_iam_member" "beepbox_reader" {
  repository = var.artifact_registry_repository_name
  location   = var.region
  role       = "roles/artifactregistry.reader"
  member     = "serviceAccount:${google_service_account.beepbox_server.email}"
}

resource "google_project_iam_member" "secret_accessor" {
  project = var.project_id
  role    = "roles/secretmanager.secretAccessor"
  member  = "serviceAccount:${google_service_account.beepbox_server.email}"
}

resource "google_project_iam_member" "log_writer" {
  project = var.project_id
  role    = "roles/logging.logWriter"
  member  = "serviceAccount:${google_service_account.beepbox_server.email}"
}

resource "google_project_iam_member" "trace_writer" {
  project = var.project_id
  role    = "roles/cloudtrace.agent"
  member  = "serviceAccount:${google_service_account.beepbox_server.email}"
}

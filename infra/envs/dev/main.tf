module "artifact_registry" {
  source = "../../modules/artifact-registry"
  region = var.region
}

module "iam" {
  source                            = "../../modules/iam"
  project_id                        = var.project_id
  region                            = var.region
  artifact_registry_repository_name = module.artifact_registry.repository_name
}

module "secrets" {
  source = "../../modules/secrets"
}

module "cloud_run" {
  source                          = "../../modules/cloud-run"
  project_id                      = var.project_id
  region                          = var.region
  service_account_email           = module.iam.service_account_email
  artifact_registry_repository_id = module.artifact_registry.repository_id
  api_keys_secret_id              = module.secrets.api_keys_secret_id
  rate_limit_secret_id            = module.secrets.rate_limit_secret_id
  auth_endpoint                   = var.auth_endpoint
  cors_allowed_origins            = var.cors_allowed_origins
}

module "firebase_hosting" {
  source                 = "../../modules/firebase-hosting"
  project_id             = var.project_id
  region                 = var.region
  cloud_run_service_name = module.cloud_run.service_name
  custom_domain          = var.custom_domain
}

# State migration: declare the renames so Terraform 1.5+ understands the
# move from flat layout (infra/*.tf) to module layout without recreating
# resources. After the first successful `terraform plan` showing 0 changes,
# these blocks can stay (they're harmless) or be removed.
moved {
  from = google_artifact_registry_repository.beepbox
  to   = module.artifact_registry.google_artifact_registry_repository.beepbox
}

moved {
  from = google_service_account.beepbox_server
  to   = module.iam.google_service_account.beepbox_server
}

moved {
  from = google_artifact_registry_repository_iam_member.beepbox_reader
  to   = module.iam.google_artifact_registry_repository_iam_member.beepbox_reader
}

moved {
  from = google_project_iam_member.secret_accessor
  to   = module.iam.google_project_iam_member.secret_accessor
}

moved {
  from = google_project_iam_member.log_writer
  to   = module.iam.google_project_iam_member.log_writer
}

moved {
  from = google_project_iam_member.trace_writer
  to   = module.iam.google_project_iam_member.trace_writer
}

moved {
  from = google_secret_manager_secret.api_keys
  to   = module.secrets.google_secret_manager_secret.api_keys
}

moved {
  from = google_secret_manager_secret_version.api_keys_v1
  to   = module.secrets.google_secret_manager_secret_version.api_keys_v1
}

moved {
  from = google_secret_manager_secret.rate_limit
  to   = module.secrets.google_secret_manager_secret.rate_limit
}

moved {
  from = google_secret_manager_secret_version.rate_limit_v1
  to   = module.secrets.google_secret_manager_secret_version.rate_limit_v1
}

moved {
  from = google_cloud_run_v2_service.beepbox
  to   = module.cloud_run.google_cloud_run_v2_service.beepbox
}

moved {
  from = google_cloud_run_v2_service_iam_member.public
  to   = module.cloud_run.google_cloud_run_v2_service_iam_member.public
}

moved {
  from = google_firebase_hosting_site.api
  to   = module.firebase_hosting.google_firebase_hosting_site.api
}

moved {
  from = google_firebase_hosting_version.api
  to   = module.firebase_hosting.google_firebase_hosting_version.api
}

moved {
  from = google_firebase_hosting_release.api
  to   = module.firebase_hosting.google_firebase_hosting_release.api
}

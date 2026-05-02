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
  source                 = "../../modules/secrets"
  api_keys_placeholder   = "bk_prod_placeholder"
  rate_limit_placeholder = "60"
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

# Existing prod resources created out-of-band (manual deploy 2026-04-23).
# Import them so Terraform takes ownership without recreating.
import {
  to = module.artifact_registry.google_artifact_registry_repository.beepbox
  id = "projects/beeping-platform-prod/locations/europe-west1/repositories/beepbox"
}

import {
  to = module.cloud_run.google_cloud_run_v2_service.beepbox
  id = "projects/beeping-platform-prod/locations/europe-west1/services/beepbox-server"
}

import {
  to = module.cloud_run.google_cloud_run_v2_service_iam_member.public
  id = "projects/beeping-platform-prod/locations/europe-west1/services/beepbox-server roles/run.invoker allUsers"
}

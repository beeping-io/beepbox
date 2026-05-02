variable "project_id" {
  description = "GCP project ID for project-level IAM bindings"
  type        = string
}

variable "region" {
  description = "GCP region (used by Artifact Registry IAM)"
  type        = string
}

variable "artifact_registry_repository_name" {
  description = "Full Artifact Registry repository name (output from artifact-registry module)"
  type        = string
}

variable "project_id" {
  description = "GCP project ID"
  type        = string
}

variable "region" {
  description = "GCP region"
  type        = string
}

variable "image_tag" {
  description = "Docker image tag (CI updates this; Terraform ignores changes)"
  type        = string
  default     = "latest"
}

variable "service_account_email" {
  description = "Service account email used by the Cloud Run service"
  type        = string
}

variable "artifact_registry_repository_id" {
  description = "Artifact Registry repository ID hosting beepbox-server images"
  type        = string
}

variable "api_keys_secret_id" {
  description = "Secret Manager secret ID for BEEPBOX_API_KEYS"
  type        = string
}

variable "rate_limit_secret_id" {
  description = "Secret Manager secret ID for BEEPBOX_RATE_LIMIT_RPM"
  type        = string
}

variable "drain_timeout_s" {
  description = "Graceful shutdown drain timeout (seconds)"
  type        = number
  default     = 8
}

variable "auth_endpoint" {
  description = "Remote API key validator URL (Cloud Function). Empty disables remote validation."
  type        = string
  default     = ""
}

variable "cors_allowed_origins" {
  description = "Comma-separated CORS-allowed origins. Empty disables CORS."
  type        = string
  default     = ""
}

variable "min_instance_count" {
  description = "Minimum Cloud Run instances (0 = scale-to-zero)"
  type        = number
  default     = 0
}

variable "max_instance_count" {
  description = "Maximum Cloud Run instances"
  type        = number
  default     = 10
}

variable "project_id" {
  description = "GCP project ID"
  type        = string
  default     = "beeping-platform-dev"
}

variable "region" {
  description = "GCP region for all resources"
  type        = string
  default     = "europe-west1"
}

variable "image_tag" {
  description = "Docker image tag to deploy (e.g. sha-abc1234 or v0.1.0)"
  type        = string
  default     = "latest"
}

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

variable "cors_allowed_origins" {
  description = <<-EOT
    Comma-separated list of CORS-whitelisted origins for browser
    callers (BEE-1794). Empty disables CORS — server-to-server flows
    keep working unchanged. For dev, expects:
    `http://localhost:3000,https://beeping-platform-dev.web.app,https://beeping-platform-dev.firebaseapp.com`.
  EOT
  type        = string
  default     = ""
}

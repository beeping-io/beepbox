variable "project_id" {
  description = "GCP project ID"
  type        = string
}

variable "region" {
  description = "GCP region"
  type        = string
  default     = "europe-west1"
}

variable "auth_endpoint" {
  description = "Remote API key validator Cloud Function URL"
  type        = string
}

variable "cors_allowed_origins" {
  description = "CORS-allowed origins (CSV). Empty disables CORS."
  type        = string
}

variable "custom_domain" {
  description = "Custom domain (FQDN) for the Firebase Hosting site"
  type        = string
  default     = ""
}

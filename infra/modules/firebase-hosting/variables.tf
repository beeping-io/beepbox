variable "project_id" {
  description = "GCP project ID (also used as site_id prefix)"
  type        = string
}

variable "region" {
  description = "GCP region of the backing Cloud Run service"
  type        = string
}

variable "cloud_run_service_name" {
  description = "Cloud Run service name to rewrite all traffic to"
  type        = string
}

variable "custom_domain" {
  description = "Optional custom domain (FQDN) to map to this Firebase Hosting site. Empty disables custom-domain registration."
  type        = string
  default     = ""
}

variable "api_keys_placeholder" {
  description = "Placeholder value for the initial secret version. Real value set out-of-band via gcloud."
  type        = string
  default     = "bk_placeholder"
  sensitive   = true
}

variable "rate_limit_placeholder" {
  description = "Initial rate limit RPM. Updated out-of-band as needed."
  type        = string
  default     = "60"
}

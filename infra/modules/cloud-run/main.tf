resource "google_cloud_run_v2_service" "beepbox" {
  name     = "beepbox-server"
  location = var.region

  template {
    service_account = var.service_account_email

    scaling {
      min_instance_count = var.min_instance_count
      max_instance_count = var.max_instance_count
    }

    containers {
      image = "${var.region}-docker.pkg.dev/${var.project_id}/${var.artifact_registry_repository_id}/beepbox-server:${var.image_tag}"

      ports {
        container_port = 8080
      }

      resources {
        limits = {
          cpu    = "1"
          memory = "512Mi"
        }
        cpu_idle = true
      }

      env {
        name = "BEEPBOX_API_KEYS"
        value_source {
          secret_key_ref {
            secret  = var.api_keys_secret_id
            version = "latest"
          }
        }
      }

      env {
        name = "BEEPBOX_RATE_LIMIT_RPM"
        value_source {
          secret_key_ref {
            secret  = var.rate_limit_secret_id
            version = "latest"
          }
        }
      }

      env {
        name  = "BEEPBOX_DRAIN_TIMEOUT_S"
        value = tostring(var.drain_timeout_s)
      }

      env {
        name  = "BEEPBOX_AUTH_ENDPOINT"
        value = var.auth_endpoint
      }

      env {
        name  = "BEEPBOX_CORS_ALLOWED_ORIGINS"
        value = var.cors_allowed_origins
      }

      startup_probe {
        http_get {
          path = "/healthz"
          port = 8080
        }
        initial_delay_seconds = 2
        period_seconds        = 3
        failure_threshold     = 5
      }

      liveness_probe {
        http_get {
          path = "/healthz"
          port = 8080
        }
        period_seconds = 30
      }
    }

    max_instance_request_concurrency = 80
    timeout                          = "30s"
  }

  lifecycle {
    ignore_changes = [
      template[0].containers[0].image,
      client,
      client_version,
    ]
  }
}

resource "google_cloud_run_v2_service_iam_member" "public" {
  name     = google_cloud_run_v2_service.beepbox.name
  location = var.region
  role     = "roles/run.invoker"
  member   = "allUsers"
}

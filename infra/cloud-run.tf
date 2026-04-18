# beepbox-server Cloud Run service
resource "google_cloud_run_v2_service" "beepbox" {
  name     = "beepbox-server"
  location = var.region

  template {
    service_account = google_service_account.beepbox_server.email

    scaling {
      min_instance_count = 0   # Scale to zero (dev — no cost when idle)
      max_instance_count = 10
    }

    containers {
      image = "${var.region}-docker.pkg.dev/${var.project_id}/${google_artifact_registry_repository.beepbox.repository_id}/beepbox-server:${var.image_tag}"

      ports {
        container_port = 8080
      }

      resources {
        limits = {
          cpu    = "1"
          memory = "512Mi"
        }
        cpu_idle = true  # CPU only allocated during requests (saves cost)
      }

      # API keys from Secret Manager
      env {
        name = "BEEPBOX_API_KEYS"
        value_source {
          secret_key_ref {
            secret  = google_secret_manager_secret.api_keys.secret_id
            version = "latest"
          }
        }
      }

      # Rate limit from Secret Manager
      env {
        name = "BEEPBOX_RATE_LIMIT_RPM"
        value_source {
          secret_key_ref {
            secret  = google_secret_manager_secret.rate_limit.secret_id
            version = "latest"
          }
        }
      }

      env {
        name  = "BEEPBOX_DRAIN_TIMEOUT_S"
        value = "8"
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

  # Allow unauthenticated access (API key auth is handled by the app)
  lifecycle {
    ignore_changes = [
      template[0].containers[0].image,  # Updated by CI, not Terraform
    ]
  }
}

# Allow public (unauthenticated) access to Cloud Run
resource "google_cloud_run_v2_service_iam_member" "public" {
  name     = google_cloud_run_v2_service.beepbox.name
  location = var.region
  role     = "roles/run.invoker"
  member   = "allUsers"
}

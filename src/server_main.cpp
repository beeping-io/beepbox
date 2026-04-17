#include <drogon/drogon.h>
#include <BeepingCoreLib_api.h>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"
#include "beepbox/WavWriter.h"
#include "beepbox/WavReader.h"
#include "beepbox/ApiKeyAuth.h"
#include "beepbox/RateLimiter.h"
#include "beepbox/Metrics.h"
#include "beepbox/Tracing.h"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#ifndef BEEPBOX_VERSION
#define BEEPBOX_VERSION "0.0.0"
#endif
#ifndef BEEPBOX_GIT_SHA
#define BEEPBOX_GIT_SHA "unknown"
#endif
#ifndef BEEPBOX_BUILD_TIME
#define BEEPBOX_BUILD_TIME "unknown"
#endif

using namespace drogon;

// --- Metrics collector (initialized in main) ---
static beepbox::MetricsCollector* g_metrics = nullptr;

// Shared auth state (initialized in main)
static beepbox::EnvKeyStore* g_keyStore = nullptr;
static beepbox::KeyCache* g_keyCache = nullptr;
static bool g_authEnabled = false;

// Shared rate limiter (initialized in main)
static beepbox::RateLimiter* g_rateLimiter = nullptr;

// Returns true if auth passes, false if response was sent with error
static bool requireAuth(
    const drogon::HttpRequestPtr& req,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback) {
  if (!g_authEnabled) return true;

  std::string authHeader = req->getHeader("Authorization");
  auto check = beepbox::checkAuth(authHeader, *g_keyStore, *g_keyCache);

  if (check.result == beepbox::AuthResult::Ok) return true;

  auto resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody(beepbox::authErrorBody(check.result));
  resp->setStatusCode(
      static_cast<HttpStatusCode>(beepbox::authStatusCode(check.result)));
  callback(resp);
  return false;
}

// Add rate limit headers to a response
static void addRateLimitHeaders(const drogon::HttpResponsePtr& resp,
                                 const beepbox::RateLimitResult& rl) {
  if (!g_rateLimiter || !g_rateLimiter->enabled()) return;
  resp->addHeader("X-RateLimit-Limit", std::to_string(rl.limit));
  resp->addHeader("X-RateLimit-Remaining", std::to_string(rl.remaining));
  resp->addHeader("X-RateLimit-Reset", std::to_string(rl.resetAt));
}

// Returns true if rate limit passes, false if 429 was sent
static bool checkRateLimit(
    const std::string& key,
    const std::function<void(const drogon::HttpResponsePtr&)>& callback) {
  if (!g_rateLimiter || !g_rateLimiter->enabled()) return true;

  auto rl = g_rateLimiter->check(key);
  if (rl.allowed) return true;

  auto resp = HttpResponse::newHttpResponse();
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody(R"({"error":"Rate limit exceeded","hint":"Retry after )" +
                std::to_string(rl.retryAfter) + R"( seconds"})");
  resp->setStatusCode(k429TooManyRequests);
  resp->addHeader("Retry-After", std::to_string(rl.retryAfter));
  addRateLimitHeaders(resp, rl);
  callback(resp);
  return false;
}

int main() {
  // --- Auth setup ---
  static beepbox::EnvKeyStore keyStore;
  static beepbox::KeyCache keyCache;
  g_keyStore = &keyStore;
  g_keyCache = &keyCache;
  g_authEnabled = !keyStore.empty();

  // --- Metrics setup ---
  static beepbox::MetricsCollector metricsCollector;
  g_metrics = &metricsCollector;

  if (g_authEnabled) {
    std::cout << "API key authentication enabled\n";
  } else {
    std::cout << "WARNING: BEEPBOX_API_KEYS not set — auth disabled (dev mode)\n";
  }

  // --- Rate limiter setup ---
  static beepbox::RateLimiter rateLimiter = beepbox::RateLimiter::fromEnv();
  g_rateLimiter = &rateLimiter;

  if (g_rateLimiter->enabled()) {
    std::cout << "Rate limiting enabled\n";
  } else {
    std::cout << "WARNING: BEEPBOX_RATE_LIMIT_RPM not set — rate limiting disabled\n";
  }

  // --- /healthz — liveness probe ---
  app().registerHandler(
      "/healthz",
      [](const HttpRequestPtr&,
         std::function<void(const HttpResponsePtr&)>&& callback) {
        auto resp = HttpResponse::newHttpJsonResponse(
            Json::Value(Json::objectValue));
        (*resp->getJsonObject())["status"] = "ok";
        resp->setStatusCode(k200OK);
        callback(resp);
      },
      {Get});

  // --- /readyz — readiness probe ---
  app().registerHandler(
      "/readyz",
      [](const HttpRequestPtr&,
         std::function<void(const HttpResponsePtr&)>&& callback) {
        // Ready if beeping-core can create an instance
        void* core = BEEPING_Create();
        bool ready = (core != nullptr);
        if (core) BEEPING_Destroy(core);

        auto resp = HttpResponse::newHttpJsonResponse(
            Json::Value(Json::objectValue));
        (*resp->getJsonObject())["ready"] = ready;
        resp->setStatusCode(ready ? k200OK : k503ServiceUnavailable);
        callback(resp);
      },
      {Get});

  // --- /version ---
  app().registerHandler(
      "/version",
      [](const HttpRequestPtr&,
         std::function<void(const HttpResponsePtr&)>&& callback) {
        auto resp = HttpResponse::newHttpJsonResponse(
            Json::Value(Json::objectValue));
        (*resp->getJsonObject())["server"] = "beepbox-server";
        (*resp->getJsonObject())["version"] = BEEPBOX_VERSION;
        (*resp->getJsonObject())["commit"] = BEEPBOX_GIT_SHA;
        (*resp->getJsonObject())["built"] = BEEPBOX_BUILD_TIME;
        (*resp->getJsonObject())["core"] = BEEPING_GetVersion();
        resp->setStatusCode(k200OK);
        callback(resp);
      },
      {Get});

  // --- POST /v1/encode — generate beeps as WAV ---
  app().registerHandler(
      "/v1/encode",
      [](const HttpRequestPtr& req,
         std::function<void(const HttpResponsePtr&)>&& callback) {
        auto t0 = std::chrono::steady_clock::now();
        if (!requireAuth(req, callback)) return;
        std::string authHdr = req->getHeader("Authorization");
        std::string apiKey = authHdr.size() > 7 ? authHdr.substr(7) : "anonymous";
        if (!checkRateLimit(apiKey, callback)) return;

        auto [span, traceResp] = beepbox::startRequestTrace(
            "encode", req->getHeader("traceparent"));
        span.setAttribute("http.method", "POST");
        span.setAttribute("http.url", "/v1/encode");
        span.setAttribute("beepbox.key_hash", beepbox::hashKey(apiKey));

        auto recordMetrics = [&](int status) {
          double dur = std::chrono::duration<double>(
              std::chrono::steady_clock::now() - t0).count();
          if (g_metrics) g_metrics->record(apiKey, "/v1/encode", status, dur);
        };

        auto addTrace = [&](const drogon::HttpResponsePtr& resp, int status) {
          resp->addHeader("traceresponse", traceResp);
          resp->addHeader("X-Trace-Id", span.traceId());
          span.setAttribute("http.status_code", status);
          span.end(status, status >= 400 ? "error" : "");
          std::cout << span.toJson() << "\n";
        };

        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          (*resp->getJsonObject())["error"] = "Invalid JSON body";
          resp->setStatusCode(k400BadRequest);
          recordMetrics(400); addTrace(resp, 400);
          callback(resp);
          return;
        }
        const auto& json = *jsonPtr;

        // Parse params from JSON
        beepbox::Params p;
        p.key = json.get("key", "").asString();

        std::string modeStr = json.get("mode", "inaudible").asString();
        if (!beepbox::parseMode(modeStr, p.mode)) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          (*resp->getJsonObject())["error"] =
              "Invalid mode. Use: audible, inaudible, all";
          resp->setStatusCode(k400BadRequest);
          recordMetrics(400); addTrace(resp, 400);
          callback(resp);
          return;
        }

        p.sampleRate = json.get("sampleRate", 44100.0f).asFloat();
        p.duration = json.get("duration", 2.3f).asFloat();
        p.startTime = json.get("start", 0.0f).asFloat();
        p.interval = json.get("interval", 2.3f).asFloat();
        p.volumeBeepsdB = json.get("volumeBeeps", -3.0f).asFloat();

        // Validate
        auto vr = beepbox::validate(p);
        if (!vr.ok) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          Json::Value errors(Json::arrayValue);
          for (const auto& e : vr.errors) errors.append(e);
          (*resp->getJsonObject())["errors"] = errors;
          resp->setStatusCode(k400BadRequest);
          recordMetrics(400); addTrace(resp, 400);
          callback(resp);
          return;
        }

        // Generate beeps
        auto result = beepbox::generateBeeps(p);
        if (result.beepsGenerated <= 0 || result.samples.empty()) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          (*resp->getJsonObject())["error"] = "Failed to generate beeps";
          resp->setStatusCode(k500InternalServerError);
          recordMetrics(500); addTrace(resp, 500);
          callback(resp);
          return;
        }

        // Convert to WAV
        auto wav = beepbox::toWav(result.samples.data(),
                                  static_cast<int>(result.samples.size()),
                                  static_cast<int>(p.sampleRate));

        auto resp = HttpResponse::newHttpResponse();
        resp->setBody(std::string(wav.begin(), wav.end()));
        resp->setContentTypeCode(CT_CUSTOM);
        resp->addHeader("Content-Type", "audio/wav");
        resp->addHeader("X-Beeps-Generated",
                        std::to_string(result.beepsGenerated));
        resp->setStatusCode(k200OK);
        recordMetrics(200); addTrace(resp, 200);
        callback(resp);
      },
      {Post});

  // --- POST /v1/decode — decode WAV audio to payload ---
  app().registerHandler(
      "/v1/decode",
      [](const HttpRequestPtr& req,
         std::function<void(const HttpResponsePtr&)>&& callback) {
        auto t0 = std::chrono::steady_clock::now();
        if (!requireAuth(req, callback)) return;
        std::string authHdrDec = req->getHeader("Authorization");
        std::string apiKeyDec = authHdrDec.size() > 7 ? authHdrDec.substr(7) : "anonymous";
        if (!checkRateLimit(apiKeyDec, callback)) return;

        auto [spanDec, traceRespDec] = beepbox::startRequestTrace(
            "decode", req->getHeader("traceparent"));
        spanDec.setAttribute("http.method", "POST");
        spanDec.setAttribute("http.url", "/v1/decode");
        spanDec.setAttribute("beepbox.key_hash", beepbox::hashKey(apiKeyDec));

        auto recordMetrics = [&](int status) {
          double dur = std::chrono::duration<double>(
              std::chrono::steady_clock::now() - t0).count();
          if (g_metrics) g_metrics->record(apiKeyDec, "/v1/decode", status, dur);
        };

        auto addTraceDec = [&](const drogon::HttpResponsePtr& resp, int status) {
          resp->addHeader("traceresponse", traceRespDec);
          resp->addHeader("X-Trace-Id", spanDec.traceId());
          spanDec.setAttribute("http.status_code", status);
          spanDec.end(status, status >= 400 ? "error" : "");
          std::cout << spanDec.toJson() << "\n";
        };

        const auto& body = req->body();
        if (body.empty()) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          (*resp->getJsonObject())["error"] = "Empty body. Send WAV audio data.";
          resp->setStatusCode(k400BadRequest);
          recordMetrics(400); addTraceDec(resp, 400);
          callback(resp);
          return;
        }

        // Parse WAV
        auto wav = beepbox::fromWav(body.data(), body.size());
        if (!wav.valid) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          (*resp->getJsonObject())["error"] = "Invalid WAV: " + wav.error;
          resp->setStatusCode(k400BadRequest);
          recordMetrics(400); addTraceDec(resp, 400);
          callback(resp);
          return;
        }

        // Decode using ALL mode (auto-detects audible/inaudible)
        constexpr int kChunk = 1024;
        void* core = BEEPING_Create();
        BEEPING_Configure(BEEPING_MODE_ALL, static_cast<float>(wav.sampleRate),
                          kChunk, core);

        int status = -1;
        int totalSamples = static_cast<int>(wav.samples.size());
        for (int offset = 0; offset < totalSamples; offset += kChunk) {
          int chunkSize = std::min(kChunk, totalSamples - offset);
          status = BEEPING_DecodeAudioBuffer(wav.samples.data() + offset,
                                             chunkSize, core);
          if (status == -3) break;
        }

        // Flush with silence
        if (status != -3) {
          std::vector<float> silence(kChunk, 0.0f);
          for (int flush = 0; flush < 200; ++flush) {
            status = BEEPING_DecodeAudioBuffer(silence.data(), kChunk, core);
            if (status == -3) break;
          }
        }

        auto resp = HttpResponse::newHttpJsonResponse(
            Json::Value(Json::objectValue));
        auto& json = *resp->getJsonObject();

        if (status == -3) {
          char buf[256] = {};
          int rc = BEEPING_GetDecodedData(buf, core);
          if (rc > 0) {
            json["decoded"] = std::string(buf, rc);
            json["confidence"] = BEEPING_GetConfidence(core);
            json["mode"] = BEEPING_GetDecodedMode(core);
            resp->setStatusCode(k200OK);
            recordMetrics(200); addTraceDec(resp, 200);
          } else {
            json["error"] = "Decode returned invalid data";
            resp->setStatusCode(k422UnprocessableEntity);
            recordMetrics(422); addTraceDec(resp, 422);
          }
        } else {
          json["error"] = "No beeping data found in audio";
          json["hint"] = "Ensure the WAV contains encoded beeps (audible or inaudible)";
          resp->setStatusCode(k404NotFound);
          recordMetrics(404); addTraceDec(resp, 404);
        }

        BEEPING_Destroy(core);
        callback(resp);
      },
      {Post});

  // --- GET /metrics — Prometheus exposition format ---
  app().registerHandler(
      "/metrics",
      [](const HttpRequestPtr&,
         std::function<void(const HttpResponsePtr&)>&& callback) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody(g_metrics ? g_metrics->serialize() : "");
        resp->setContentTypeCode(CT_CUSTOM);
        resp->addHeader("Content-Type",
                        "text/plain; version=0.0.4; charset=utf-8");
        resp->setStatusCode(k200OK);
        callback(resp);
      },
      {Get});

  // --- Graceful shutdown ---
  auto shutdownHandler = [](int sig) {
    std::cout << "{\"severity\":\"INFO\",\"message\":\"Received signal "
              << sig << ", shutting down...\"}\n";
    app().quit();
  };
  std::signal(SIGTERM, shutdownHandler);
  std::signal(SIGINT, shutdownHandler);

  // Drain timeout: how long to wait for active connections before exit
  const char* drainEnv = std::getenv("BEEPBOX_DRAIN_TIMEOUT_S");
  int drainTimeout = drainEnv ? std::atoi(drainEnv) : 8;
  if (drainTimeout < 1) drainTimeout = 8;
  app().setIdleConnectionTimeout(drainTimeout);

  std::cout << "beepbox-server starting on 0.0.0.0:8080 (drain timeout: "
            << drainTimeout << "s)\n";
  app().addListener("0.0.0.0", 8080);
  app().setThreadNum(4);
  app().run();

  std::cout << "{\"severity\":\"INFO\",\"message\":\"Shutdown complete\"}\n";
  return 0;
}

#include <drogon/drogon.h>
#include <BeepingCoreLib_api.h>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"
#include "beepbox/WavWriter.h"

#include <iostream>
#include <string>

using namespace drogon;

int main() {
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
        (*resp->getJsonObject())["version"] = "0.0.0";
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
        auto jsonPtr = req->getJsonObject();
        if (!jsonPtr) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          (*resp->getJsonObject())["error"] = "Invalid JSON body";
          resp->setStatusCode(k400BadRequest);
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
        callback(resp);
      },
      {Post});

  std::cout << "beepbox-server starting on 0.0.0.0:8080\n";
  app().addListener("0.0.0.0", 8080);
  app().setThreadNum(4);
  app().run();

  return 0;
}

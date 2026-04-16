#include <drogon/drogon.h>
#include <BeepingCoreLib_api.h>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"
#include "beepbox/WavWriter.h"
#include "beepbox/WavReader.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

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

  // --- POST /v1/decode — decode WAV audio to payload ---
  app().registerHandler(
      "/v1/decode",
      [](const HttpRequestPtr& req,
         std::function<void(const HttpResponsePtr&)>&& callback) {
        const auto& body = req->body();
        if (body.empty()) {
          auto resp = HttpResponse::newHttpJsonResponse(
              Json::Value(Json::objectValue));
          (*resp->getJsonObject())["error"] = "Empty body. Send WAV audio data.";
          resp->setStatusCode(k400BadRequest);
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
          } else {
            json["error"] = "Decode returned invalid data";
            resp->setStatusCode(k422UnprocessableEntity);
          }
        } else {
          json["error"] = "No beeping data found in audio";
          json["hint"] = "Ensure the WAV contains encoded beeps (audible or inaudible)";
          resp->setStatusCode(k404NotFound);
        }

        BEEPING_Destroy(core);
        callback(resp);
      },
      {Post});

  std::cout << "beepbox-server starting on 0.0.0.0:8080\n";
  app().addListener("0.0.0.0", 8080);
  app().setThreadNum(4);
  app().run();

  return 0;
}

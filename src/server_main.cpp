#include <drogon/drogon.h>
#include <BeepingCoreLib_api.h>
#include "beepbox/Generator.h"
#include "beepbox/Params.h"

#include <iostream>

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

  std::cout << "beepbox-server starting on 0.0.0.0:8080\n";
  app().addListener("0.0.0.0", 8080);
  app().setThreadNum(4);
  app().run();

  return 0;
}

// SPDX-License-Identifier: Apache-2.0
// Copyright Beeping contributors

#include "beepbox/CorsConfig.h"

#include <drogon/drogon.h>

namespace beepbox {

void installCorsHandlers(CorsConfig cfg) {
  if (!cfg.enabled()) return;

  // PreRoutingAdvice: catch OPTIONS preflights before auth/rate-limit
  // filters. The browser sends OPTIONS without the Bearer token, so if
  // we let it fall through to /v1/encode the auth handler would 401 it
  // and the actual CORS preflight would never succeed.
  drogon::app().registerPreRoutingAdvice(
      [cfg](const drogon::HttpRequestPtr& req,
            drogon::AdviceCallback&& callback,
            drogon::AdviceChainCallback&& chain) {
        if (req->method() != drogon::Options) {
          chain();
          return;
        }
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        const auto origin = req->getHeader("Origin");
        if (auto allowed = cfg.resolveAllowOrigin(origin); allowed) {
          resp->addHeader("Access-Control-Allow-Origin", *allowed);
          resp->addHeader("Access-Control-Allow-Methods",
                          std::string(CorsConfig::kAllowMethods));
          resp->addHeader("Access-Control-Allow-Headers",
                          std::string(CorsConfig::kAllowHeaders));
          resp->addHeader("Access-Control-Max-Age",
                          std::string(CorsConfig::kMaxAge));
          resp->addHeader("Vary", "Origin");
        }
        callback(resp);
      });

  // PostHandlingAdvice: stamp Allow-Origin on every non-OPTIONS response
  // (POST /v1/encode, GET /healthz, …) when the request originated from
  // a whitelisted browser. No-op for server-to-server callers because
  // they don't send an `Origin` header.
  drogon::app().registerPostHandlingAdvice(
      [cfg](const drogon::HttpRequestPtr& req,
            const drogon::HttpResponsePtr& resp) {
        if (req->method() == drogon::Options) return;
        const auto origin = req->getHeader("Origin");
        if (auto allowed = cfg.resolveAllowOrigin(origin); allowed) {
          resp->addHeader("Access-Control-Allow-Origin", *allowed);
          resp->addHeader("Vary", "Origin");
        }
      });
}

} // namespace beepbox

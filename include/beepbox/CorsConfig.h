// SPDX-License-Identifier: Apache-2.0
// Copyright Beeping contributors

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace beepbox {

/**
 * Pure logic that decides which CORS Allow-Origin to echo back.
 *
 * Backed by an exact-match whitelist parsed from a CSV env var
 * (BEEPBOX_CORS_ALLOWED_ORIGINS). No wildcards, no regex, no scheme
 * coercion — if the request `Origin` header doesn't match a configured
 * entry verbatim, no CORS headers are emitted and the browser blocks
 * the response naturally.
 *
 * When the whitelist is empty (env var unset or empty), CORS is
 * effectively OFF — which is the backwards-compatible default for
 * the existing server-to-server flows that don't need CORS.
 *
 * Drogon integration lives in CorsConfig.cpp behind installCorsHandlers().
 * The pure parsing and resolution logic is exposed here so it can be
 * unit-tested without spinning up the HTTP server.
 */
class CorsConfig {
public:
  /// Parse a CSV (comma-separated origins). Trims surrounding whitespace
  /// per entry and drops empty entries. Order of origins in the input is
  /// preserved.
  static std::vector<std::string> parseAllowedOrigins(std::string_view csv);

  CorsConfig() = default;
  explicit CorsConfig(std::vector<std::string> allowedOrigins);

  /// True if at least one origin is whitelisted. When false, callers
  /// should skip installCorsHandlers() entirely so Drogon's request
  /// pipeline is unchanged for the legacy server-to-server flow.
  bool enabled() const noexcept { return !allowedOrigins_.empty(); }

  const std::vector<std::string>& allowedOrigins() const noexcept {
    return allowedOrigins_;
  }

  /// Returns the value to echo back as `Access-Control-Allow-Origin`
  /// for a given request `Origin` header, or std::nullopt if the
  /// origin is not whitelisted (or empty/missing).
  std::optional<std::string> resolveAllowOrigin(
      std::string_view requestOrigin) const;

  /// Static headers — same on every preflight response.
  static constexpr std::string_view kAllowMethods =
      "GET, POST, OPTIONS";
  static constexpr std::string_view kAllowHeaders =
      "Authorization, Content-Type, traceparent";
  static constexpr std::string_view kMaxAge = "600";

private:
  std::vector<std::string> allowedOrigins_;
};

/**
 * Hook the CORS config into Drogon's request pipeline.
 *
 * Registers two pieces of advice (the two integration points Drogon
 * exposes for cross-cutting concerns):
 *
 *   1. preRoutingAdvice → intercepts OPTIONS preflight requests before
 *      any handler or auth filter sees them. If the request `Origin`
 *      is whitelisted, responds 204 with the four `Access-Control-*`
 *      headers; otherwise responds 204 with no CORS headers and the
 *      browser refuses the actual request.
 *
 *   2. postHandlingAdvice → appends `Access-Control-Allow-Origin` (and
 *      `Vary: Origin`) to every non-OPTIONS response when the request
 *      came from a whitelisted origin. Skipped for any other origin so
 *      server-to-server callers (no Origin header) see no behavioural
 *      change.
 *
 * Calling this when `cfg.enabled() == false` is a no-op so it's safe
 * to invoke unconditionally at startup.
 */
void installCorsHandlers(CorsConfig cfg);

} // namespace beepbox

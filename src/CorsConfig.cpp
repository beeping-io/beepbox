// SPDX-License-Identifier: Apache-2.0
// Copyright Beeping contributors

#include "beepbox/CorsConfig.h"

#include <cctype>
#include <utility>

namespace beepbox {

namespace {

std::string_view trim(std::string_view sv) {
  const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!sv.empty() && isSpace(static_cast<unsigned char>(sv.front()))) {
    sv.remove_prefix(1);
  }
  while (!sv.empty() && isSpace(static_cast<unsigned char>(sv.back()))) {
    sv.remove_suffix(1);
  }
  return sv;
}

} // namespace

std::vector<std::string> CorsConfig::parseAllowedOrigins(
    std::string_view csv) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= csv.size()) {
    std::size_t comma = csv.find(',', start);
    std::string_view chunk = csv.substr(
        start, comma == std::string_view::npos ? csv.size() - start
                                               : comma - start);
    auto trimmed = trim(chunk);
    if (!trimmed.empty()) {
      out.emplace_back(trimmed);
    }
    if (comma == std::string_view::npos) break;
    start = comma + 1;
  }
  return out;
}

CorsConfig::CorsConfig(std::vector<std::string> allowedOrigins)
    : allowedOrigins_(std::move(allowedOrigins)) {}

std::optional<std::string> CorsConfig::resolveAllowOrigin(
    std::string_view requestOrigin) const {
  if (requestOrigin.empty()) return std::nullopt;
  for (const auto& whitelisted : allowedOrigins_) {
    if (requestOrigin == whitelisted) {
      return whitelisted;
    }
  }
  return std::nullopt;
}

} // namespace beepbox

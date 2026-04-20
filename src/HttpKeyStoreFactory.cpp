// Server-only implementation of the HttpKeyStore factory.
// Pulls in cpp-httplib (HTTPS client) and jsoncpp (response parsing).
// Kept out of the core library so tests don't need OpenSSL/jsoncpp.

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <json/json.h>

#include <sstream>
#include <stdexcept>

#include "beepbox/HttpKeyStore.h"

namespace beepbox {

namespace {

/// Splits "https://host/path" into ("https://host", "/path").
/// If `url` has no path beyond the host, returns ("https://host", "/").
std::pair<std::string, std::string> splitEndpoint(const std::string& url) {
  auto schemeEnd = url.find("://");
  if (schemeEnd == std::string::npos) {
    throw std::invalid_argument(
        "makeHttpKeyStoreFromEndpoint: endpoint must be a full URL");
  }
  auto pathStart = url.find('/', schemeEnd + 3);
  if (pathStart == std::string::npos) return {url, "/"};
  return {url.substr(0, pathStart), url.substr(pathStart)};
}

}  // namespace

HttpKeyStore makeHttpKeyStoreFromEndpoint(const std::string& endpoint,
                                           int timeoutSeconds) {
  auto [base, path] = splitEndpoint(endpoint);
  return HttpKeyStore([base, path, timeoutSeconds](
                          const std::string& key) -> bool {
    try {
      httplib::Client client(base.c_str());
      client.set_connection_timeout(timeoutSeconds, 0);
      client.set_read_timeout(timeoutSeconds, 0);
      client.set_write_timeout(timeoutSeconds, 0);
      client.enable_server_certificate_verification(true);

      Json::Value body(Json::objectValue);
      body["key"] = key;
      Json::StreamWriterBuilder writer;
      writer.settings_["indentation"] = "";
      const std::string bodyStr = Json::writeString(writer, body);

      auto res = client.Post(path.c_str(), bodyStr, "application/json");
      if (!res || res->status != 200) return false;

      Json::CharReaderBuilder reader;
      Json::Value root;
      std::string errs;
      std::istringstream iss(res->body);
      if (!Json::parseFromStream(reader, iss, &root, &errs)) return false;
      return root.isMember("valid") && root["valid"].isBool() &&
             root["valid"].asBool();
    } catch (...) {
      return false;
    }
  });
}

}  // namespace beepbox

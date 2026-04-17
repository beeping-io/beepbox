#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdio>
#include <memory>
#include <string>


static constexpr const char* kBaseUrl = "http://127.0.0.1:8080";

namespace {

struct CurlResult {
  int httpStatus;
  std::string body;
};

// Run curl and capture output + HTTP status code
CurlResult curlGet(const std::string& path) {
  std::string cmd = "curl -s -o /tmp/beepbox_test_body.txt "
                    "-w '%{http_code}' " + std::string(kBaseUrl) + path +
                    " 2>/dev/null";
  std::array<char, 16> buf{};
  std::string statusStr;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) return {-1, ""};
  while (fgets(buf.data(), buf.size(), pipe.get()))
    statusStr += buf.data();

  // Read body
  std::string body;
  FILE* f = fopen("/tmp/beepbox_test_body.txt", "rb");
  if (f) {
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    body.resize(len);
    fread(body.data(), 1, len, f);
    fclose(f);
  }

  int status = statusStr.empty() ? -1 : std::stoi(statusStr);
  return {status, body};
}

CurlResult curlPostJson(const std::string& path, const std::string& json) {
  std::string cmd = "curl -s -o /tmp/beepbox_test_body.txt "
                    "-w '%{http_code}' "
                    "-X POST -H 'Content-Type: application/json' "
                    "-d '" + json + "' " +
                    std::string(kBaseUrl) + path + " 2>/dev/null";
  std::array<char, 16> buf{};
  std::string statusStr;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) return {-1, ""};
  while (fgets(buf.data(), buf.size(), pipe.get()))
    statusStr += buf.data();

  std::string body;
  FILE* f = fopen("/tmp/beepbox_test_body.txt", "rb");
  if (f) {
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    body.resize(len);
    fread(body.data(), 1, len, f);
    fclose(f);
  }

  int status = statusStr.empty() ? -1 : std::stoi(statusStr);
  return {status, body};
}

CurlResult curlPostBinary(const std::string& path,
                           const std::string& filePath) {
  std::string cmd = "curl -s -o /tmp/beepbox_test_body.txt "
                    "-w '%{http_code}' "
                    "-X POST -H 'Content-Type: audio/wav' "
                    "--data-binary @" + filePath + " " +
                    std::string(kBaseUrl) + path + " 2>/dev/null";
  std::array<char, 16> buf{};
  std::string statusStr;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) return {-1, ""};
  while (fgets(buf.data(), buf.size(), pipe.get()))
    statusStr += buf.data();

  std::string body;
  FILE* f = fopen("/tmp/beepbox_test_body.txt", "rb");
  if (f) {
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    body.resize(len);
    fread(body.data(), 1, len, f);
    fclose(f);
  }

  int status = statusStr.empty() ? -1 : std::stoi(statusStr);
  return {status, body};
}

bool serverRunning() {
  auto r = curlGet("/healthz");
  return r.httpStatus == 200;
}

}  // namespace

// All tests require beepbox-server running on port 8080.
// Start it before running: ./build/beepbox-server &

TEST_CASE("GET /healthz returns 200 with status ok", "[server][healthz]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  auto r = curlGet("/healthz");
  REQUIRE(r.httpStatus == 200);
  REQUIRE(r.body.find("\"status\"") != std::string::npos);
  REQUIRE(r.body.find("\"ok\"") != std::string::npos);
}

TEST_CASE("GET /readyz returns 200 with ready field", "[server][readyz]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  auto r = curlGet("/readyz");
  REQUIRE(r.httpStatus == 200);
  REQUIRE(r.body.find("\"ready\"") != std::string::npos);
  REQUIRE(r.body.find("true") != std::string::npos);
}

TEST_CASE("GET /version returns server metadata", "[server][version]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  auto r = curlGet("/version");
  REQUIRE(r.httpStatus == 200);
  REQUIRE(r.body.find("\"server\"") != std::string::npos);
  REQUIRE(r.body.find("\"beepbox-server\"") != std::string::npos);
  REQUIRE(r.body.find("\"version\"") != std::string::npos);
  REQUIRE(r.body.find("\"commit\"") != std::string::npos);
  REQUIRE(r.body.find("\"built\"") != std::string::npos);
  REQUIRE(r.body.find("\"core\"") != std::string::npos);
}

TEST_CASE("GET /metrics returns Prometheus format", "[server][metrics]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  auto r = curlGet("/metrics");
  REQUIRE(r.httpStatus == 200);
  REQUIRE(r.body.find("# HELP beepbox_uptime_seconds") != std::string::npos);
  REQUIRE(r.body.find("# TYPE beepbox_uptime_seconds gauge") != std::string::npos);
  REQUIRE(r.body.find("beepbox_requests_total") != std::string::npos);
  REQUIRE(r.body.find("beepbox_encode_total") != std::string::npos);
  REQUIRE(r.body.find("beepbox_decode_total") != std::string::npos);
}

TEST_CASE("POST /v1/encode with valid key returns WAV", "[server][encode]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  auto r = curlPostJson("/v1/encode",
      R"({"key": "a1b2c", "mode": "inaudible", "duration": 2.3})");
  REQUIRE(r.httpStatus == 200);
  REQUIRE(r.body.size() > 44);
  REQUIRE(r.body.substr(0, 4) == "RIFF");
}

TEST_CASE("POST /v1/encode with invalid key returns 400", "[server][encode]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  auto r = curlPostJson("/v1/encode", R"({"key": "XYZ"})");
  REQUIRE(r.httpStatus == 400);
  REQUIRE(r.body.find("error") != std::string::npos);
}

TEST_CASE("POST /v1/encode with invalid mode returns 400", "[server][encode]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  auto r = curlPostJson("/v1/encode",
      R"({"key": "a1b2c", "mode": "ultrasonic"})");
  REQUIRE(r.httpStatus == 400);
  REQUIRE(r.body.find("Invalid mode") != std::string::npos);
}

TEST_CASE("POST /v1/decode with empty body returns 400", "[server][decode]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  // Send empty file
  system("touch /tmp/beepbox_empty.wav");
  auto r = curlPostBinary("/v1/decode", "/tmp/beepbox_empty.wav");
  REQUIRE(r.httpStatus == 400);
}

TEST_CASE("POST /v1/decode with invalid WAV returns 400", "[server][decode]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }
  system("echo 'not-a-wav' > /tmp/beepbox_bad.wav");
  auto r = curlPostBinary("/v1/decode", "/tmp/beepbox_bad.wav");
  REQUIRE(r.httpStatus == 400);
  REQUIRE(r.body.find("Invalid WAV") != std::string::npos);
}

TEST_CASE("Encode then Decode round-trip via HTTP", "[server][roundtrip]") {
  if (!serverRunning()) { WARN("Server not running — skipping"); return; }

  // Encode — save WAV to file
  std::string encCmd = "curl -s -o /tmp/beepbox_rt.wav "
                       "-w '%{http_code}' "
                       "-X POST -H 'Content-Type: application/json' "
                       "-d '{\"key\": \"a1b2c\", \"mode\": \"audible\", \"duration\": 2.3}' "
                       + std::string(kBaseUrl) + "/v1/encode 2>/dev/null";
  std::array<char, 16> buf{};
  std::string statusStr;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(encCmd.c_str(), "r"), pclose);
  while (fgets(buf.data(), buf.size(), pipe.get()))
    statusStr += buf.data();
  REQUIRE(std::stoi(statusStr) == 200);

  // Decode the WAV
  auto r = curlPostBinary("/v1/decode", "/tmp/beepbox_rt.wav");
  REQUIRE(r.httpStatus == 200);
  REQUIRE(r.body.find("\"decoded\"") != std::string::npos);
  // beeping-core returns payload with zero-padding (e.g. "a1b2c0000")
  REQUIRE(r.body.find("a1b2c") != std::string::npos);
}

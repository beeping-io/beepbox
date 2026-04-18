#include <catch2/catch_test_macros.hpp>
#include "beepbox/Metrics.h"

// --- hashKey ---

TEST_CASE("hashKey is deterministic", "[metrics]") {
  auto h1 = beepbox::hashKey("bk_test123");
  auto h2 = beepbox::hashKey("bk_test123");
  REQUIRE(h1 == h2);
  REQUIRE(h1.size() == 8);
}

TEST_CASE("hashKey differs for different keys", "[metrics]") {
  auto h1 = beepbox::hashKey("bk_aaa");
  auto h2 = beepbox::hashKey("bk_bbb");
  REQUIRE(h1 != h2);
}

// --- Histogram ---

TEST_CASE("Histogram counts observations in correct buckets", "[metrics]") {
  beepbox::Histogram h;

  h.observe(0.005);  // bucket 0.01
  h.observe(0.03);   // bucket 0.05
  h.observe(0.5);    // bucket 0.5
  h.observe(7.0);    // bucket 10

  REQUIRE(h.count() == 4);
  REQUIRE(h.sum() > 7.5);

  auto output = h.serialize("test_duration", "endpoint=\"/test\"");
  REQUIRE(output.find("test_duration_count") != std::string::npos);
  REQUIRE(output.find("test_duration_sum") != std::string::npos);
  REQUIRE(output.find("le=\"+Inf\"") != std::string::npos);
}

TEST_CASE("Histogram +Inf bucket catches all", "[metrics]") {
  beepbox::Histogram h;
  h.observe(999.0);  // way beyond all buckets

  REQUIRE(h.count() == 1);
  auto output = h.serialize("test", "x=\"y\"");
  REQUIRE(output.find("le=\"+Inf\"} 1") != std::string::npos);
}

// --- MetricsCollector ---

TEST_CASE("MetricsCollector records and increments counters", "[metrics]") {
  beepbox::MetricsCollector mc;

  mc.record("bk_key1", "/v1/encode", 200, 0.1);
  mc.record("bk_key1", "/v1/encode", 200, 0.2);
  mc.record("bk_key1", "/v1/encode", 400, 0.01);

  auto kh = beepbox::hashKey("bk_key1");
  REQUIRE(mc.quotaConsumed(kh) == 3);
}

TEST_CASE("Per-key isolation in metrics", "[metrics]") {
  beepbox::MetricsCollector mc;

  mc.record("bk_alpha", "/v1/encode", 200, 0.1);
  mc.record("bk_alpha", "/v1/encode", 200, 0.1);
  mc.record("bk_beta", "/v1/decode", 200, 0.5);

  auto ha = beepbox::hashKey("bk_alpha");
  auto hb = beepbox::hashKey("bk_beta");
  REQUIRE(mc.quotaConsumed(ha) == 2);
  REQUIRE(mc.quotaConsumed(hb) == 1);
}

TEST_CASE("Serialize produces valid Prometheus output", "[metrics]") {
  beepbox::MetricsCollector mc;

  mc.record("bk_prom", "/v1/encode", 200, 0.05);

  auto output = mc.serialize();

  // Uptime
  REQUIRE(output.find("# HELP beepbox_uptime_seconds") != std::string::npos);
  REQUIRE(output.find("# TYPE beepbox_uptime_seconds gauge") != std::string::npos);

  // Per-key counters
  REQUIRE(output.find("beepbox_requests_total{key_hash=") != std::string::npos);
  REQUIRE(output.find("endpoint=\"/v1/encode\"") != std::string::npos);
  REQUIRE(output.find("status=\"200\"") != std::string::npos);

  // Quota
  REQUIRE(output.find("beepbox_quota_consumed_total{key_hash=") != std::string::npos);

  // Duration histogram
  REQUIRE(output.find("beepbox_request_duration_seconds_bucket{") != std::string::npos);
  REQUIRE(output.find("beepbox_request_duration_seconds_count{") != std::string::npos);
  REQUIRE(output.find("beepbox_request_duration_seconds_sum{") != std::string::npos);
}

TEST_CASE("Empty collector serializes without crash", "[metrics]") {
  beepbox::MetricsCollector mc;
  auto output = mc.serialize();
  REQUIRE(output.find("beepbox_uptime_seconds") != std::string::npos);
  REQUIRE(output.find("beepbox_requests_total 0") != std::string::npos);
}

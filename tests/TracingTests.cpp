#include <catch2/catch_test_macros.hpp>
#include "beepbox/Tracing.h"

// --- traceparent parsing ---

TEST_CASE("Valid traceparent is parsed correctly", "[tracing]") {
  auto ctx = beepbox::parseTraceparent(
      "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
  REQUIRE(ctx.valid);
  REQUIRE(ctx.traceId == "4bf92f3577b34da6a3ce929d0e0e4736");
  REQUIRE(ctx.parentSpanId == "00f067aa0ba902b7");
  REQUIRE(ctx.traceFlags == 0x01);
}

TEST_CASE("Empty traceparent is invalid", "[tracing]") {
  auto ctx = beepbox::parseTraceparent("");
  REQUIRE_FALSE(ctx.valid);
}

TEST_CASE("Malformed traceparent is invalid", "[tracing]") {
  auto ctx = beepbox::parseTraceparent("not-a-traceparent");
  REQUIRE_FALSE(ctx.valid);
}

TEST_CASE("All-zero trace ID is invalid", "[tracing]") {
  auto ctx = beepbox::parseTraceparent(
      "00-00000000000000000000000000000000-00f067aa0ba902b7-01");
  REQUIRE_FALSE(ctx.valid);
}

TEST_CASE("All-zero span ID is invalid", "[tracing]") {
  auto ctx = beepbox::parseTraceparent(
      "00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01");
  REQUIRE_FALSE(ctx.valid);
}

TEST_CASE("Unsupported version is invalid", "[tracing]") {
  auto ctx = beepbox::parseTraceparent(
      "ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
  REQUIRE_FALSE(ctx.valid);
}

// --- ID generation ---

TEST_CASE("generateTraceId returns 32 hex chars", "[tracing]") {
  auto id = beepbox::generateTraceId();
  REQUIRE(id.size() == 32);
  for (char c : id) {
    REQUIRE(((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')));
  }
}

TEST_CASE("generateSpanId returns 16 hex chars", "[tracing]") {
  auto id = beepbox::generateSpanId();
  REQUIRE(id.size() == 16);
}

TEST_CASE("Generated IDs are unique", "[tracing]") {
  auto a = beepbox::generateTraceId();
  auto b = beepbox::generateTraceId();
  REQUIRE(a != b);
}

// --- formatTraceparent ---

TEST_CASE("formatTraceparent round-trips", "[tracing]") {
  std::string tp = beepbox::formatTraceparent(
      "4bf92f3577b34da6a3ce929d0e0e4736", "00f067aa0ba902b7", 0x01);
  REQUIRE(tp == "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");

  auto ctx = beepbox::parseTraceparent(tp);
  REQUIRE(ctx.valid);
  REQUIRE(ctx.traceId == "4bf92f3577b34da6a3ce929d0e0e4736");
}

// --- Span ---

TEST_CASE("Span records duration", "[tracing]") {
  beepbox::Span span("test-span", beepbox::generateTraceId(), "");
  span.setAttribute("key", "value");
  span.end();
  REQUIRE(span.ended());
  REQUIRE(span.durationMs() >= 0.0);
  REQUIRE(span.statusCode() == 200);
}

TEST_CASE("Span end with error sets status", "[tracing]") {
  beepbox::Span span("fail-span", beepbox::generateTraceId(), "");
  span.end(500, "internal error");
  REQUIRE(span.statusCode() == 500);
}

TEST_CASE("Span toJson produces valid JSON structure", "[tracing]") {
  beepbox::Span span("json-span", "abcd1234abcd1234abcd1234abcd1234", "parent1234567890");
  span.setAttribute("http.method", "POST");
  span.end();

  auto json = span.toJson();
  REQUIRE(json.find("\"severity\":\"INFO\"") != std::string::npos);
  REQUIRE(json.find("\"traceId\":\"abcd1234abcd1234abcd1234abcd1234\"") != std::string::npos);
  REQUIRE(json.find("\"parentSpanId\":\"parent1234567890\"") != std::string::npos);
  REQUIRE(json.find("\"http.method\":\"POST\"") != std::string::npos);
  REQUIRE(json.find("\"durationMs\":") != std::string::npos);
}

TEST_CASE("Span double-end is safe", "[tracing]") {
  beepbox::Span span("double", beepbox::generateTraceId(), "");
  span.end();
  auto d1 = span.durationMs();
  span.end();  // should be no-op
  REQUIRE(span.durationMs() == d1);
}

// --- startRequestTrace ---

TEST_CASE("startRequestTrace with traceparent preserves trace ID", "[tracing]") {
  auto [span, tp] = beepbox::startRequestTrace(
      "test", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
  REQUIRE(span.traceId() == "4bf92f3577b34da6a3ce929d0e0e4736");
  REQUIRE(span.parentSpanId() == "00f067aa0ba902b7");
  REQUIRE(tp.find("4bf92f3577b34da6a3ce929d0e0e4736") != std::string::npos);
}

TEST_CASE("startRequestTrace without traceparent generates new trace", "[tracing]") {
  auto [span, tp] = beepbox::startRequestTrace("test", "");
  REQUIRE(span.traceId().size() == 32);
  REQUIRE(span.parentSpanId().empty());
  REQUIRE(!tp.empty());
}

TEST_CASE("GCP trace resource format", "[tracing]") {
  beepbox::Span span("gcp", "abcd1234abcd1234abcd1234abcd1234", "");
  auto res = span.gcpTraceResource("my-project");
  REQUIRE(res == "projects/my-project/traces/abcd1234abcd1234abcd1234abcd1234");
}

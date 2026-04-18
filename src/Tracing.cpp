#include "beepbox/Tracing.h"

#include <iomanip>
#include <random>
#include <sstream>

namespace beepbox {

// --- Random hex generation ---

static thread_local std::mt19937_64 g_rng{std::random_device{}()};

static std::string randomHex(size_t numChars) {
  std::ostringstream ss;
  ss << std::hex << std::setfill('0');
  // Generate 8 hex chars at a time from 32-bit chunks
  size_t remaining = numChars;
  while (remaining > 0) {
    uint64_t val = g_rng();
    size_t chars = std::min(remaining, size_t(16));
    ss << std::setw(static_cast<int>(chars))
       << (val & ((chars == 16) ? ~0ULL : ((1ULL << (chars * 4)) - 1)));
    remaining -= chars;
  }
  return ss.str().substr(0, numChars);
}

std::string generateTraceId() { return randomHex(32); }
std::string generateSpanId() { return randomHex(16); }

// --- traceparent parsing ---

static bool isHex(const std::string& s) {
  for (char c : s) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

TraceContext parseTraceparent(const std::string& header) {
  TraceContext ctx;

  // Format: version-traceId-parentSpanId-traceFlags
  // Example: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
  if (header.size() < 55) return ctx;  // minimum valid length

  if (header[2] != '-' || header[35] != '-' || header[52] != '-') return ctx;

  std::string version = header.substr(0, 2);
  std::string traceId = header.substr(3, 32);
  std::string parentSpanId = header.substr(36, 16);
  std::string flags = header.substr(53, 2);

  if (!isHex(version) || !isHex(traceId) || !isHex(parentSpanId) || !isHex(flags))
    return ctx;

  // Version 00 is the only one we support
  if (version != "00") return ctx;

  // All-zero trace ID or span ID is invalid
  if (traceId == "00000000000000000000000000000000") return ctx;
  if (parentSpanId == "0000000000000000") return ctx;

  ctx.traceId = traceId;
  ctx.parentSpanId = parentSpanId;
  ctx.traceFlags = static_cast<uint8_t>(std::stoi(flags, nullptr, 16));
  ctx.valid = true;
  return ctx;
}

std::string formatTraceparent(const std::string& traceId,
                               const std::string& spanId,
                               uint8_t traceFlags) {
  std::ostringstream ss;
  ss << "00-" << traceId << "-" << spanId << "-"
     << std::hex << std::setfill('0') << std::setw(2)
     << static_cast<int>(traceFlags);
  return ss.str();
}

// --- Span ---

Span::Span(const std::string& name,
           const std::string& traceId,
           const std::string& parentSpanId)
    : name_(name),
      traceId_(traceId),
      spanId_(generateSpanId()),
      parentSpanId_(parentSpanId),
      startTime_(std::chrono::steady_clock::now()) {}

void Span::setAttribute(const std::string& key, const std::string& value) {
  attributes_[key] = value;
}

void Span::setAttribute(const std::string& key, int value) {
  attributes_[key] = std::to_string(value);
}

void Span::end() {
  if (ended_) return;
  ended_ = true;
  durationMs_ = std::chrono::duration<double, std::milli>(
      std::chrono::steady_clock::now() - startTime_).count();
}

void Span::end(int code, const std::string& err) {
  statusCode_ = code;
  error_ = err;
  end();
}

std::string Span::toJson() const {
  std::ostringstream ss;
  ss << "{";
  ss << "\"severity\":\"INFO\",";
  ss << "\"message\":\"" << name_ << "\",";
  ss << "\"logging.googleapis.com/spanId\":\"" << spanId_ << "\",";
  ss << "\"logging.googleapis.com/trace_sampled\":true,";
  ss << "\"span\":{";
  ss << "\"name\":\"" << name_ << "\",";
  ss << "\"traceId\":\"" << traceId_ << "\",";
  ss << "\"spanId\":\"" << spanId_ << "\",";
  ss << "\"parentSpanId\":\"" << parentSpanId_ << "\",";
  ss << "\"durationMs\":" << std::fixed << std::setprecision(2) << durationMs_ << ",";
  ss << "\"statusCode\":" << statusCode_;

  if (!error_.empty()) {
    ss << ",\"error\":\"" << error_ << "\"";
  }

  if (!attributes_.empty()) {
    ss << ",\"attributes\":{";
    bool first = true;
    for (const auto& [k, v] : attributes_) {
      if (!first) ss << ",";
      ss << "\"" << k << "\":\"" << v << "\"";
      first = false;
    }
    ss << "}";
  }

  ss << "}}";
  return ss.str();
}

std::string Span::gcpTraceResource(const std::string& projectId) const {
  return "projects/" + projectId + "/traces/" + traceId_;
}

// --- Request tracing ---

RequestTrace startRequestTrace(const std::string& spanName,
                                const std::string& traceparentHeader) {
  auto ctx = parseTraceparent(traceparentHeader);

  std::string traceId = ctx.valid ? ctx.traceId : generateTraceId();
  std::string parentSpanId = ctx.valid ? ctx.parentSpanId : "";
  uint8_t flags = ctx.valid ? ctx.traceFlags : 0x01;

  Span span(spanName, traceId, parentSpanId);
  std::string responseTraceparent = formatTraceparent(traceId, span.spanId(), flags);

  return {std::move(span), std::move(responseTraceparent)};
}

}  // namespace beepbox

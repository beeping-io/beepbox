#ifndef BEEPBOX_TRACING_H
#define BEEPBOX_TRACING_H

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace beepbox {

/// Parsed W3C traceparent header.
/// Format: version-traceId-parentSpanId-traceFlags
/// Example: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
struct TraceContext {
  std::string traceId;       // 32 hex chars
  std::string parentSpanId;  // 16 hex chars
  uint8_t traceFlags = 0;    // 0x01 = sampled
  bool valid = false;
};

/// Parse a W3C traceparent header value.
TraceContext parseTraceparent(const std::string& header);

/// Format a W3C traceparent header value.
std::string formatTraceparent(const std::string& traceId,
                               const std::string& spanId,
                               uint8_t traceFlags);

/// Generate a random 32-char hex trace ID.
std::string generateTraceId();

/// Generate a random 16-char hex span ID.
std::string generateSpanId();

/// A lightweight span for request tracing.
class Span {
 public:
  Span(const std::string& name,
       const std::string& traceId,
       const std::string& parentSpanId);

  /// Set an attribute on this span.
  void setAttribute(const std::string& key, const std::string& value);
  void setAttribute(const std::string& key, int value);

  /// End the span and record its duration.
  void end();

  /// End the span with an error status.
  void end(int statusCode, const std::string& error);

  const std::string& name() const { return name_; }
  const std::string& traceId() const { return traceId_; }
  const std::string& spanId() const { return spanId_; }
  const std::string& parentSpanId() const { return parentSpanId_; }
  double durationMs() const { return durationMs_; }
  int statusCode() const { return statusCode_; }
  bool ended() const { return ended_; }

  /// Serialize to a JSON string for structured logging.
  /// Includes fields compatible with Cloud Trace auto-detection.
  std::string toJson() const;

  /// Get the GCP trace resource name for structured logging.
  /// Format: projects/PROJECT_ID/traces/TRACE_ID
  std::string gcpTraceResource(const std::string& projectId) const;

 private:
  std::string name_;
  std::string traceId_;
  std::string spanId_;
  std::string parentSpanId_;
  std::chrono::steady_clock::time_point startTime_;
  double durationMs_ = 0.0;
  int statusCode_ = 200;
  std::string error_;
  bool ended_ = false;
  std::unordered_map<std::string, std::string> attributes_;
};

/// Create a span from an incoming request, extracting or generating trace context.
/// Returns the span and the traceparent to set on the response.
struct RequestTrace {
  Span span;
  std::string responseTraceparent;
};

RequestTrace startRequestTrace(const std::string& spanName,
                                const std::string& traceparentHeader);

}  // namespace beepbox

#endif  // BEEPBOX_TRACING_H

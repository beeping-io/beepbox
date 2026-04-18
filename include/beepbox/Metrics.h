#ifndef BEEPBOX_METRICS_H
#define BEEPBOX_METRICS_H

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace beepbox {

/// Hash an API key to an 8-char hex string for privacy in metrics labels.
std::string hashKey(const std::string& key);

/// Histogram with fixed buckets.
class Histogram {
 public:
  Histogram();

  /// Observe a value (e.g. request duration in seconds).
  void observe(double value);

  /// Serialize to Prometheus exposition format lines.
  /// @param name Metric name (e.g. "beepbox_request_duration_seconds")
  /// @param labels Label string (e.g. {key_hash="abc",endpoint="/v1/encode"})
  std::string serialize(const std::string& name,
                        const std::string& labels) const;

  uint64_t count() const { return count_; }
  double sum() const { return sum_; }

  static const std::vector<double>& buckets();

 private:
  std::vector<uint64_t> bucketCounts_;  // one per bucket + +Inf
  uint64_t count_ = 0;
  double sum_ = 0.0;
};

/// Per-endpoint per-status counter key.
struct CounterKey {
  std::string keyHash;
  std::string endpoint;
  int status;

  bool operator==(const CounterKey& o) const {
    return keyHash == o.keyHash && endpoint == o.endpoint && status == o.status;
  }
};

struct CounterKeyHash {
  size_t operator()(const CounterKey& k) const {
    size_t h = std::hash<std::string>{}(k.keyHash);
    h ^= std::hash<std::string>{}(k.endpoint) << 1;
    h ^= std::hash<int>{}(k.status) << 2;
    return h;
  }
};

/// Per-key histogram key.
struct HistoKey {
  std::string keyHash;
  std::string endpoint;

  bool operator==(const HistoKey& o) const {
    return keyHash == o.keyHash && endpoint == o.endpoint;
  }
};

struct HistoKeyHash {
  size_t operator()(const HistoKey& k) const {
    size_t h = std::hash<std::string>{}(k.keyHash);
    h ^= std::hash<std::string>{}(k.endpoint) << 1;
    return h;
  }
};

/// Thread-safe metrics collector for the beepbox server.
class MetricsCollector {
 public:
  MetricsCollector();

  /// Record a completed request.
  void record(const std::string& apiKey,
              const std::string& endpoint,
              int statusCode,
              double durationSeconds);

  /// Get total quota consumed for a key (all endpoints, all statuses).
  uint64_t quotaConsumed(const std::string& keyHash) const;

  /// Serialize all metrics to Prometheus exposition format.
  std::string serialize() const;

 private:
  mutable std::mutex mu_;
  std::chrono::steady_clock::time_point startTime_;

  // Per-key per-endpoint per-status counters
  std::unordered_map<CounterKey, uint64_t, CounterKeyHash> counters_;

  // Per-key per-endpoint duration histograms
  std::unordered_map<HistoKey, Histogram, HistoKeyHash> histograms_;

  // Per-key total quota consumed
  std::unordered_map<std::string, uint64_t> quota_;
};

}  // namespace beepbox

#endif  // BEEPBOX_METRICS_H

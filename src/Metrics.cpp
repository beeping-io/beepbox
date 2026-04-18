#include "beepbox/Metrics.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <sstream>

namespace beepbox {

// --- hashKey ---

std::string hashKey(const std::string& key) {
  // Simple FNV-1a hash truncated to 8 hex chars.
  // Not cryptographic — just for label cardinality reduction.
  uint64_t hash = 14695981039346656037ULL;
  for (char c : key) {
    hash ^= static_cast<uint64_t>(c);
    hash *= 1099511628211ULL;
  }
  std::ostringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(8)
     << (hash & 0xFFFFFFFF);
  return ss.str();
}

// --- Histogram ---

static const std::vector<double> kBuckets = {
    0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};

const std::vector<double>& Histogram::buckets() { return kBuckets; }

Histogram::Histogram() : bucketCounts_(kBuckets.size() + 1, 0) {}

void Histogram::observe(double value) {
  count_++;
  sum_ += value;
  for (size_t i = 0; i < kBuckets.size(); ++i) {
    if (value <= kBuckets[i]) {
      bucketCounts_[i]++;
      return;
    }
  }
  bucketCounts_.back()++;  // +Inf
}

std::string Histogram::serialize(const std::string& name,
                                  const std::string& labels) const {
  std::ostringstream ss;
  // Cumulative buckets
  uint64_t cumulative = 0;
  for (size_t i = 0; i < kBuckets.size(); ++i) {
    cumulative += bucketCounts_[i];
    ss << name << "_bucket{" << labels << ",le=\"" << kBuckets[i] << "\"} "
       << cumulative << "\n";
  }
  cumulative += bucketCounts_.back();
  ss << name << "_bucket{" << labels << ",le=\"+Inf\"} "
     << cumulative << "\n";
  ss << name << "_sum{" << labels << "} " << sum_ << "\n";
  ss << name << "_count{" << labels << "} " << count_ << "\n";
  return ss.str();
}

// --- MetricsCollector ---

MetricsCollector::MetricsCollector()
    : startTime_(std::chrono::steady_clock::now()) {}

void MetricsCollector::record(const std::string& apiKey,
                               const std::string& endpoint,
                               int statusCode,
                               double durationSeconds) {
  std::string kh = hashKey(apiKey);
  std::lock_guard<std::mutex> lock(mu_);

  counters_[{kh, endpoint, statusCode}]++;
  histograms_[{kh, endpoint}].observe(durationSeconds);
  quota_[kh]++;
}

uint64_t MetricsCollector::quotaConsumed(const std::string& keyHash) const {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = quota_.find(keyHash);
  return it != quota_.end() ? it->second : 0;
}

std::string MetricsCollector::serialize() const {
  std::lock_guard<std::mutex> lock(mu_);
  std::ostringstream ss;

  // Uptime
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::steady_clock::now() - startTime_);
  ss << "# HELP beepbox_uptime_seconds Time since server start.\n";
  ss << "# TYPE beepbox_uptime_seconds gauge\n";
  ss << "beepbox_uptime_seconds " << elapsed.count() << "\n\n";

  // Per-key per-endpoint per-status request counters
  ss << "# HELP beepbox_requests_total Total HTTP requests by key, endpoint, status.\n";
  ss << "# TYPE beepbox_requests_total counter\n";
  uint64_t globalTotal = 0;
  for (const auto& [key, count] : counters_) {
    ss << "beepbox_requests_total{key_hash=\"" << key.keyHash
       << "\",endpoint=\"" << key.endpoint
       << "\",status=\"" << key.status << "\"} " << count << "\n";
    globalTotal += count;
  }
  if (counters_.empty()) {
    ss << "beepbox_requests_total 0\n";
  }
  ss << "\n";

  // Per-key quota consumed
  ss << "# HELP beepbox_quota_consumed_total Total requests consumed per API key.\n";
  ss << "# TYPE beepbox_quota_consumed_total counter\n";
  for (const auto& [kh, count] : quota_) {
    ss << "beepbox_quota_consumed_total{key_hash=\"" << kh << "\"} "
       << count << "\n";
  }
  if (quota_.empty()) {
    ss << "beepbox_quota_consumed_total 0\n";
  }
  ss << "\n";

  // Per-key per-endpoint duration histograms
  ss << "# HELP beepbox_request_duration_seconds Request duration in seconds.\n";
  ss << "# TYPE beepbox_request_duration_seconds histogram\n";
  for (const auto& [key, histo] : histograms_) {
    std::string labels = "key_hash=\"" + key.keyHash +
                         "\",endpoint=\"" + key.endpoint + "\"";
    ss << histo.serialize("beepbox_request_duration_seconds", labels);
  }
  ss << "\n";

  return ss.str();
}

}  // namespace beepbox

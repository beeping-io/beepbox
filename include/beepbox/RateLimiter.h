#ifndef BEEPBOX_RATE_LIMITER_H
#define BEEPBOX_RATE_LIMITER_H

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace beepbox {

/// Result of a rate limit check.
struct RateLimitResult {
  bool allowed;       // Whether the request is allowed
  int limit;          // Max requests per window
  int remaining;      // Tokens remaining
  int64_t resetAt;    // Unix timestamp when bucket refills to full
  int retryAfter;     // Seconds to wait (only meaningful if !allowed)
};

/// Token bucket for a single key.
class TokenBucket {
 public:
  TokenBucket(double capacity, double refillPerSecond);

  /// Try to consume one token. Returns current state.
  RateLimitResult consume();

 private:
  void refill();

  double capacity_;
  double tokens_;
  double refillPerSecond_;
  std::chrono::steady_clock::time_point lastRefill_;
};

/// Thread-safe per-key rate limiter using token buckets.
class RateLimiter {
 public:
  /// Create a rate limiter.
  /// @param requestsPerMinute Max requests per minute per key (bucket capacity).
  explicit RateLimiter(int requestsPerMinute);

  /// Check and consume a token for the given key.
  RateLimitResult check(const std::string& key);

  /// Whether rate limiting is active.
  bool enabled() const { return enabled_; }

  /// Number of keys currently tracked.
  size_t keyCount() const;

  /// Create from environment variable BEEPBOX_RATE_LIMIT_RPM.
  /// Returns a limiter with enabled()=false if not set.
  static RateLimiter fromEnv();

 private:
  bool enabled_;
  int rpm_;
  mutable std::mutex mu_;
  std::unordered_map<std::string, TokenBucket> buckets_;
};

}  // namespace beepbox

#endif  // BEEPBOX_RATE_LIMITER_H

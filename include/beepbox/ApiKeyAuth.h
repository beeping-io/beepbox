#ifndef BEEPBOX_API_KEY_AUTH_H
#define BEEPBOX_API_KEY_AUTH_H

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace beepbox {

/// Abstract key store — pluggable backend for API key validation.
class KeyStore {
 public:
  virtual ~KeyStore() = default;
  /// Returns true if the key is valid.
  virtual bool validate(const std::string& key) = 0;
};

/// Reads keys from the BEEPBOX_API_KEYS environment variable (comma-separated).
class EnvKeyStore : public KeyStore {
 public:
  EnvKeyStore();
  bool validate(const std::string& key) override;
  bool empty() const { return keys_.empty(); }

 private:
  std::unordered_set<std::string> keys_;
};

/// Thread-safe LRU cache with TTL for API key validation results.
class KeyCache {
 public:
  explicit KeyCache(size_t maxSize = 1000,
                    std::chrono::seconds ttl = std::chrono::seconds(300));

  /// Check cache. Returns {found, valid}.
  std::pair<bool, bool> get(const std::string& key);

  /// Insert or update a cache entry.
  void put(const std::string& key, bool valid);

  /// Number of entries currently cached.
  size_t size() const;

 private:
  struct Entry {
    bool valid;
    std::chrono::steady_clock::time_point expires;
  };

  void evictExpired();

  mutable std::mutex mu_;
  size_t maxSize_;
  std::chrono::seconds ttl_;
  std::unordered_map<std::string, Entry> entries_;
};

/// Result of parsing an Authorization header.
enum class AuthResult {
  Ok,            // Key is valid
  Missing,       // No Authorization header
  BadFormat,     // Header present but not "Bearer bk_..."
  InvalidKey,    // Key not found in store
  AuthDisabled,  // No keys configured (dev mode)
};

/// Parse the Authorization header and validate the key.
/// Returns AuthResult and the extracted key (if any).
struct AuthCheck {
  AuthResult result;
  std::string key;
};

AuthCheck checkAuth(const std::string& authHeader,
                    KeyStore& store,
                    KeyCache& cache);

/// HTTP status code for a given AuthResult.
int authStatusCode(AuthResult r);

/// JSON error body for a given AuthResult.
std::string authErrorBody(AuthResult r);

}  // namespace beepbox

#endif  // BEEPBOX_API_KEY_AUTH_H

#include "beepbox/ApiKeyAuth.h"

#include <cstdlib>
#include <sstream>

namespace beepbox {

// --- EnvKeyStore ---

EnvKeyStore::EnvKeyStore() {
  const char* env = std::getenv("BEEPBOX_API_KEYS");
  if (!env) return;
  std::istringstream ss(env);
  std::string token;
  while (std::getline(ss, token, ',')) {
    // Trim whitespace
    auto start = token.find_first_not_of(" \t");
    auto end = token.find_last_not_of(" \t");
    if (start != std::string::npos)
      keys_.insert(token.substr(start, end - start + 1));
  }
}

bool EnvKeyStore::validate(const std::string& key) {
  return keys_.count(key) > 0;
}

// --- KeyCache ---

KeyCache::KeyCache(size_t maxSize, std::chrono::seconds ttl)
    : maxSize_(maxSize), ttl_(ttl) {}

std::pair<bool, bool> KeyCache::get(const std::string& key) {
  std::lock_guard<std::mutex> lock(mu_);
  auto it = entries_.find(key);
  if (it == entries_.end()) return {false, false};
  if (std::chrono::steady_clock::now() > it->second.expires) {
    entries_.erase(it);
    return {false, false};
  }
  return {true, it->second.valid};
}

void KeyCache::put(const std::string& key, bool valid) {
  std::lock_guard<std::mutex> lock(mu_);
  if (entries_.size() >= maxSize_) {
    evictExpired();
    // If still full, remove an arbitrary entry
    if (entries_.size() >= maxSize_) {
      entries_.erase(entries_.begin());
    }
  }
  entries_[key] = {valid,
                   std::chrono::steady_clock::now() + ttl_};
}

size_t KeyCache::size() const {
  std::lock_guard<std::mutex> lock(mu_);
  return entries_.size();
}

void KeyCache::evictExpired() {
  auto now = std::chrono::steady_clock::now();
  for (auto it = entries_.begin(); it != entries_.end();) {
    if (now > it->second.expires)
      it = entries_.erase(it);
    else
      ++it;
  }
}

// --- Auth check ---

static const std::string kBearerPrefix = "Bearer ";
static const std::string kKeyPrefix = "bk_";

AuthCheck checkAuth(const std::string& authHeader,
                    KeyStore& store,
                    KeyCache& cache) {
  if (authHeader.empty()) {
    return {AuthResult::Missing, ""};
  }

  // Must start with "Bearer "
  if (authHeader.size() < kBearerPrefix.size() ||
      authHeader.compare(0, kBearerPrefix.size(), kBearerPrefix) != 0) {
    return {AuthResult::BadFormat, ""};
  }

  std::string key = authHeader.substr(kBearerPrefix.size());

  // Must start with "bk_"
  if (key.size() < kKeyPrefix.size() ||
      key.compare(0, kKeyPrefix.size(), kKeyPrefix) != 0) {
    return {AuthResult::BadFormat, key};
  }

  // Check cache first
  auto [found, valid] = cache.get(key);
  if (found) {
    return {valid ? AuthResult::Ok : AuthResult::InvalidKey, key};
  }

  // Lookup in store
  bool isValid = store.validate(key);
  cache.put(key, isValid);

  return {isValid ? AuthResult::Ok : AuthResult::InvalidKey, key};
}

int authStatusCode(AuthResult r) {
  switch (r) {
    case AuthResult::Missing:
    case AuthResult::BadFormat:
      return 401;
    case AuthResult::InvalidKey:
      return 403;
    default:
      return 200;
  }
}

std::string authErrorBody(AuthResult r) {
  switch (r) {
    case AuthResult::Missing:
      return R"({"error":"Missing API key","hint":"Add header: Authorization: Bearer bk_..."})";
    case AuthResult::BadFormat:
      return R"({"error":"Invalid API key format","hint":"Key must start with bk_"})";
    case AuthResult::InvalidKey:
      return R"({"error":"Invalid API key"})";
    default:
      return "{}";
  }
}

}  // namespace beepbox

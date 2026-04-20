#ifndef BEEPBOX_HTTP_KEY_STORE_H
#define BEEPBOX_HTTP_KEY_STORE_H

#include <functional>
#include <string>

#include "beepbox/ApiKeyAuth.h"

namespace beepbox {

/// KeyStore that delegates validation to an injected callable.
///
/// The callable takes a raw key (e.g. "bk_XXXXXX") and returns true iff the
/// key is currently valid. Implementations backed by a remote HTTP service
/// MUST fail closed (return false) on network/timeout/5xx errors.
///
/// Production code uses `makeHttpKeyStoreFromEndpoint()` to build one backed
/// by a POST to a Cloud Function validator; tests inject a lambda directly.
class HttpKeyStore : public KeyStore {
 public:
  using Validator = std::function<bool(const std::string& key)>;

  explicit HttpKeyStore(Validator fn) : fn_(std::move(fn)) {}

  bool validate(const std::string& key) override { return fn_(key); }

 private:
  Validator fn_;
};

/// Factory — builds an HttpKeyStore that POSTs `{"key": "..."}` to
/// `{endpoint}` and interprets a JSON body of the form `{"valid": bool,...}`.
///
/// - `timeoutSeconds`: hard timeout per request. Default 3 s.
/// - Any non-200 response, timeout, DNS error, or JSON parse failure is
///   treated as INVALID (fail closed).
///
/// Defined in `src/HttpKeyStoreFactory.cpp` so that tests and callers that
/// only need the class body avoid pulling in the cpp-httplib / OpenSSL
/// dependencies.
HttpKeyStore makeHttpKeyStoreFromEndpoint(const std::string& endpoint,
                                           int timeoutSeconds = 3);

}  // namespace beepbox

#endif  // BEEPBOX_HTTP_KEY_STORE_H

/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/security.h"

namespace roq {
namespace kraken {

Security::Security(const Config &config, const std::string_view &account)
    : account_(account), hasher_(
                             config.get_access_key(account),
                             config.get_access_secret(account),
                             config.get_access_password(account)) {
}

std::string Security::create_body() {
  return hasher_.create_body();
}

std::string Security::create_headers(
    core::http::Method method, const std::string_view &path, const std::string_view &body) {
  return hasher_.create_headers(method, path, body);
}

}  // namespace kraken
}  // namespace roq

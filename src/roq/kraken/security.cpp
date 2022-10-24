/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/security.hpp"

namespace roq {
namespace kraken {

// === IMPLEMENTATION ===

Security::Security(Config const &config, std::string_view const &account)
    : account_{account}, hasher_{
                             config.get_access_key(account),
                             config.get_access_secret(account),
                             config.get_access_password(account)} {
}

std::string Security::create_body() {
  return hasher_.create_body();
}

std::string Security::create_headers(
    web::http::Method method, std::string_view const &path, std::string_view const &body) {
  return hasher_.create_headers(method, path, body);
}

}  // namespace kraken
}  // namespace roq

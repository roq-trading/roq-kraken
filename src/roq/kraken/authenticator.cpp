/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/kraken/authenticator.hpp"

namespace roq {
namespace kraken {

// === IMPLEMENTATION ===

Authenticator::Authenticator(Config const &config, std::string_view const &account)
    : account_{account},
      crypto_{config.get_access_key(account), config.get_access_secret(account), config.get_access_password(account)} {
}

std::string Authenticator::create_body() {
  return crypto_.create_body();
}

std::string Authenticator::create_headers(
    web::http::Method method, std::string_view const &path, std::string_view const &body) {
  return crypto_.create_headers(method, path, body);
}

}  // namespace kraken
}  // namespace roq

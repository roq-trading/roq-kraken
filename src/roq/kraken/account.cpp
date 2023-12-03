/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/kraken/account.hpp"

namespace roq {
namespace kraken {

// === IMPLEMENTATION ===

Account::Account(Config const &config, std::string_view const &name)
    : name_{name},
      crypto_{config.get_access_key(name), config.get_access_secret(name), config.get_access_password(name)} {
}

std::string Account::create_body() {
  return crypto_.create_body();
}

std::string Account::create_headers(
    web::http::Method method, std::string_view const &path, std::string_view const &body) {
  return crypto_.create_headers(method, path, body);
}

}  // namespace kraken
}  // namespace roq

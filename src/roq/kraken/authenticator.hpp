/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/web/http/method.hpp"

#include "roq/kraken/config.hpp"

#include "roq/kraken/tools/crypto.hpp"

namespace roq {
namespace kraken {

struct Authenticator final {
  Authenticator(Config const &, std::string_view const &account);

  Authenticator(Authenticator &&) = delete;
  Authenticator(Authenticator const &) = delete;

  std::string_view get_account() const { return account_; }

  std::string create_body();

  std::string create_headers(web::http::Method, std::string_view const &path, std::string_view const &body);

 private:
  std::string const account_;
  tools::Crypto crypto_;
};

}  // namespace kraken
}  // namespace roq

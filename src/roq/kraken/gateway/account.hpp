/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/web/http/method.hpp"

#include "roq/kraken/gateway/config.hpp"

#include "roq/kraken/tools/crypto.hpp"

namespace roq {
namespace kraken {
namespace gateway {

struct Account final {
  Account(Config const &, std::string_view const &name);

  Account(Account const &) = delete;

  std::string create_body();

  std::string create_headers(web::http::Method, std::string_view const &path, std::string_view const &body);

  std::string const name;

 private:
  tools::Crypto crypto_;
};

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

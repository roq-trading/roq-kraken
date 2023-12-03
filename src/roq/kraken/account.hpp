/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/web/http/method.hpp"

#include "roq/kraken/config.hpp"

#include "roq/kraken/tools/crypto.hpp"

namespace roq {
namespace kraken {

struct Account final {
  Account(Config const &, std::string_view const &name);

  Account(Account &&) = delete;
  Account(Account const &) = delete;

  std::string_view get_name() const { return name_; }

  std::string create_body();

  std::string create_headers(web::http::Method, std::string_view const &path, std::string_view const &body);

 private:
  std::string const name_;
  tools::Crypto crypto_;
};

}  // namespace kraken
}  // namespace roq

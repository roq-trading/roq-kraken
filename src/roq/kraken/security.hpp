/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/web/http/method.hpp"

#include "roq/kraken/config.hpp"

#include "roq/kraken/tools/hasher.hpp"

namespace roq {
namespace kraken {

class Security final {
 public:
  Security(Config const &, std::string_view const &account);

  Security(Security &&) = delete;
  Security(Security const &) = delete;

  std::string_view get_account() const { return account_; }

  std::string create_body();

  std::string create_headers(web::http::Method, std::string_view const &path, std::string_view const &body);

 private:
  const std::string account_;
  tools::Hasher hasher_;
};

}  // namespace kraken
}  // namespace roq

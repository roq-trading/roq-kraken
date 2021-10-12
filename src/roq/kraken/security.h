/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>

#include "roq/core/http/method.h"

#include "roq/kraken/config.h"

#include "roq/kraken/tools/hasher.h"

namespace roq {
namespace kraken {

class Security final {
 public:
  Security(const Config &, const std::string_view &account);

  Security(Security &&) = delete;
  Security(const Security &) = delete;

  std::string_view get_account() const { return account_; }

  std::string create_body();

  std::string create_headers(
      core::http::Method, const std::string_view &path, const std::string_view &body);

 private:
  const std::string account_;
  tools::Hasher hasher_;
};

}  // namespace kraken
}  // namespace roq

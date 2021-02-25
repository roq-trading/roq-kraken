/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>

#include "roq/core/crypto/hmac.h"
#include "roq/core/crypto/sha.h"

#include "roq/core/http/method.h"

#include "roq/kraken/config.h"

namespace roq {
namespace kraken {

class Security final {
 public:
  explicit Security(const Config &);

  Security(Security &&) = delete;
  Security(const Security &) = delete;

  std::string create_body();

  std::string create_headers(
      const core::http::Method &method, const std::string_view &path, const std::string_view &body);

 private:
  const std::string key_;
  const std::string password_;
  core::crypto::SHA256 sha_;
  core::crypto::HMAC_SHA512 hmac_;
  // experimental
  std::chrono::milliseconds nonce_ = {};
};

}  // namespace kraken
}  // namespace roq

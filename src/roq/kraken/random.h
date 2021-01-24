/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>

#include "roq/core/crypto/hmac.h"
#include "roq/core/crypto/sha.h"

#include "roq/core/http/method.h"

namespace roq {
namespace kraken {

class Random final {
 public:
  explicit Random(
      const std::string_view &key,
      const std::string_view &secret,
      const std::string_view &password);

  Random(Random &&) = delete;
  Random(const Random &) = delete;

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

/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include "roq/core/crypto/hmac.h"
#include "roq/core/crypto/sha.h"

#include "roq/core/http/method.h"

#include "roq/kraken/config.h"

namespace roq {
namespace kraken {
namespace tools {

class Hasher final {
 public:
  Hasher(
      const std::string_view &key,
      const std::string_view &secret,
      const std::string_view &passphrase);

  Hasher(Hasher &&) = delete;
  Hasher(const Hasher &) = delete;

  std::string create_body();

  std::string create_headers(
      core::http::Method, const std::string_view &path, const std::string_view &body);

 private:
  const std::string key_;
  const std::string passphrase_;
  core::crypto::SHA256 sha_;
  core::crypto::HMAC_SHA512 hmac_;
  // experimental
  std::chrono::milliseconds nonce_ = {};
};

}  // namespace tools
}  // namespace kraken
}  // namespace roq

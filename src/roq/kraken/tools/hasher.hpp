/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include "roq/core/crypto/hmac_sha512.hpp"
#include "roq/core/crypto/sha256.hpp"

#include "roq/web/http/method.hpp"

#include "roq/kraken/config.hpp"

namespace roq {
namespace kraken {
namespace tools {

class Hasher final {
 public:
  Hasher(std::string_view const &key, std::string_view const &secret, std::string_view const &passphrase);

  Hasher(Hasher &&) = delete;
  Hasher(Hasher const &) = delete;

  std::string create_body();

  std::string create_headers(web::http::Method, std::string_view const &path, std::string_view const &body);

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

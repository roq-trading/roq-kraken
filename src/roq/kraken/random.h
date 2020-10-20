/* Copyright (c) 2017-2020, Hans Erik Thrane */

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
      const core::http::Method &method,
      const std::string_view &path,
      const std::string_view &body);

 private:
  const std::string _key;
  const std::string _password;
  core::crypto::SHA256 _sha;
  core::crypto::HMAC_SHA512 _hmac;
  // experimental
  std::chrono::milliseconds _nonce = {};
};

}  // namespace kraken
}  // namespace roq

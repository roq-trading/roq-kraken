/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <array>
#include <chrono>
#include <string>
#include <string_view>

#include "roq/core/hash/sha256.hpp"

#include "roq/core/mac/hmac.hpp"

#include "roq/web/http/method.hpp"

#include "roq/kraken/config.hpp"

namespace roq {
namespace kraken {
namespace tools {

struct Hasher final {
  Hasher(std::string_view const &key, std::string_view const &secret, std::string_view const &passphrase);

  Hasher(Hasher &&) = delete;
  Hasher(Hasher const &) = delete;

  std::string create_body();

  std::string create_headers(web::http::Method, std::string_view const &path, std::string_view const &body);

 private:
  using Hash = core::hash::SHA256;
  using MAC = core::mac::HMAC<core::hash::SHA512>;
  using Digest = std::array<std::byte, MAC::DIGEST_LENGTH>;

  std::string const key_;
  std::string const passphrase_;
  Hash hash_;
  MAC mac_;
  Digest digest_;
  // experimental
  std::chrono::milliseconds nonce_ = {};
};

}  // namespace tools
}  // namespace kraken
}  // namespace roq

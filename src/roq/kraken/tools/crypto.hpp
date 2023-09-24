/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <array>
#include <chrono>
#include <string>
#include <string_view>

#include "roq/utils/hash/sha256.hpp"

#include "roq/utils/mac/hmac.hpp"

#include "roq/web/http/method.hpp"

#include "roq/kraken/config.hpp"

namespace roq {
namespace kraken {
namespace tools {

struct Crypto final {
  Crypto(std::string_view const &key, std::string_view const &secret, std::string_view const &passphrase);

  Crypto(Crypto &&) = delete;
  Crypto(Crypto const &) = delete;

  std::string create_body();

  std::string create_headers(web::http::Method, std::string_view const &path, std::string_view const &body);

 private:
  using Hash = utils::hash::SHA256;
  using MAC = utils::mac::HMAC<utils::hash::SHA512>;
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

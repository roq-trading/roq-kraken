/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/tools/hasher.hpp"

#include <array>
#include <random>

#include "roq/logging.hpp"

#include "roq/core/clock.hpp"

#include "roq/core/binascii/base64.hpp"

using namespace std::literals;
using namespace std::literals;

namespace roq {
namespace kraken {
namespace tools {

namespace {
const constexpr auto THRESHOLD = -1000ms;

auto create_hmac(std::string_view const &secret) {
  auto raw_secret = core::binascii::Base64::decode(secret, true);
  return core::crypto::HMAC_SHA512(raw_secret);
}
}  // namespace

Hasher::Hasher(std::string_view const &key, std::string_view const &secret, std::string_view const &passphrase)
    : key_(key), passphrase_(passphrase), hmac_(create_hmac(secret)) {
}

std::string Hasher::create_body() {
  auto now = std::chrono::duration_cast<decltype(nonce_)>(core::clock::GetRealTime());
  auto diff = now - nonce_;
  if (diff < THRESHOLD) [[unlikely]]
    log::fatal("Probably something wrong... diff={})"sv, diff);
  if (diff.count() < 0)  // XXX shouldn't this be <= ?
    ++nonce_;
  else
    nonce_ = now;
  if (std::empty(passphrase_)) {
    return fmt::format(R"(nonce={})"sv, nonce_.count());
  } else {
    // XXX something weird with the quotes here... review
    return fmt::format(
        R"("nonce={}&)"
        R"("opt={}")"sv,
        nonce_.count(),
        passphrase_);
  }
}

std::string Hasher::create_headers(
    web::http::Method method, std::string_view const &path, std::string_view const &body) {
  assert(method == web::http::Method::POST);
  assert(!std::empty(body));
  auto nonce = fmt::format("{}"sv, nonce_.count());
  sha_.clear();
  sha_.update(nonce);
  sha_.update(body);
  std::array<char, 32> buffer_1;
  auto length_1 = sha_.digest(buffer_1);
  assert(length_1 == std::size(buffer_1));
  hmac_.clear();
  hmac_.update(std::data(path), std::size(path));
  hmac_.update(std::data(buffer_1), std::size(buffer_1));
  std::array<char, 64> buffer_2;
  auto length_2 = hmac_.digest(buffer_2);
  assert(length_2 == std::size(buffer_2));
  auto sign_2 = core::binascii::Base64::encode(buffer_2, false);
  return fmt::format(
      "API-Key: {}\r\n"
      "API-Sign: {}\r\n"sv,
      key_,
      sign_2);
}

}  // namespace tools
}  // namespace kraken
}  // namespace roq

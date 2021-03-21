/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/security.h"

#include <array>
#include <random>

#include "roq/logging.h"

#include "roq/core/clock.h"

#include "roq/core/binascii/base64.h"

#include "roq/core/crypto/sha.h"

using namespace std::literals;
using namespace roq::literals;

namespace roq {
namespace kraken {

namespace {
static const constexpr auto THRESHOLD = -1000ms;

static auto create_hmac(const std::string_view &secret) {
  auto raw_secret = core::binascii::Base64::decode(secret, true);
  return core::crypto::HMAC_SHA512(raw_secret);
}
}  // namespace

Security::Security(const Config &config, const std::string_view &account)
    : account_(account), key_(config.get_access_key(account)),
      password_(config.get_access_password(account)),
      hmac_(create_hmac(config.get_access_secret(account))) {
}

std::string Security::create_body() {
  auto now = std::chrono::duration_cast<decltype(nonce_)>(core::get_realtime_clock());
  auto diff = now - nonce_;
  if (ROQ_UNLIKELY(diff < THRESHOLD))
    log::fatal("Probably something wrong... diff={})"_fmt, diff);
  if (diff.count() < 0)  // XXX shouldn't this be <= ?
    ++nonce_;
  else
    nonce_ = now;
  if (password_.empty()) {
    return roq::format(R"(nonce={})"_fmt, nonce_.count());
  } else {
    // XXX something weird with the quotes here... review
    return roq::format(
        R"("nonce={}&)"
        R"("opt={}")"_fmt,
        nonce_.count(),
        password_);
  }
}

std::string Security::create_headers(
    const core::http::Method &method, const std::string_view &path, const std::string_view &body) {
  assert(method == core::http::Method::POST);
  assert(!body.empty());
  auto nonce = roq::format("{}"_fmt, nonce_.count());
  sha_.clear();
  sha_.update(nonce);
  sha_.update(body);
  std::array<char, 32u> buffer_1;
  auto length_1 = sha_.digest(buffer_1);
  assert(length_1 == buffer_1.size());
  hmac_.clear();
  hmac_.update(path.data(), path.length());
  hmac_.update(buffer_1.data(), buffer_1.size());
  std::array<char, 64u> buffer_2;
  auto length_2 = hmac_.digest(buffer_2);
  assert(length_2 == buffer_2.size());
  auto sign_2 = core::binascii::Base64::encode(buffer_2);
  return roq::format(
      "API-Key: {}\r\n"
      "API-Sign: {}\r\n"_fmt,
      key_,
      sign_2);
}

}  // namespace kraken
}  // namespace roq

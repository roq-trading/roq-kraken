/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/random.h"

#include <fmt/format.h>

#include <array>
#include <random>

#include "roq/logging.h"

#include "roq/builtins.h"

#include "roq/core/clock.h"

#include "roq/core/binascii/base64.h"

#include "roq/core/crypto/sha.h"

namespace roq {
namespace kraken {

namespace {
constexpr int64_t THRESHOLD = -1000;
static auto create_hmac(const std::string_view& secret) {
  auto raw_secret = core::binascii::Base64::decode(
      secret.data(),
      secret.length(),
      true);
  return core::crypto::HMAC_SHA512(
      raw_secret.data(),
      raw_secret.size());
}
}  // namespace

Random::Random(
    const std::string_view& key,
    const std::string_view& secret,
    const std::string_view& password)
    : _key(key),
      _password(password),
      _hmac(create_hmac(secret)) {
}

std::string Random::create_body() {
  auto now = std::chrono::duration_cast<decltype(_nonce)>(
      core::get_realtime_clock());
  auto diff = (now - _nonce).count();
  LOG_IF(FATAL, diff < THRESHOLD)(
      FMT_STRING(R"(Probably something wrong... diff={})"),
      diff);
  if (diff < 0)
    ++_nonce;
  else
    _nonce = now;
  if (_password.empty()) {
    return fmt::format(
        FMT_STRING(R"(nonce={})"),
        _nonce.count());
  } else {
    return fmt::format(
        FMT_STRING(
            R"("nonce={}&)"
            R"("opt={}")"),
        _nonce.count(),
        _password);
  }
}

std::string Random::create_headers(
    const core::http::Method& method,
    const std::string_view& path,
    const std::string_view& body) {
  assert(method == core::http::Method::POST);
  assert(body.empty() == false);
  auto nonce = fmt::format(
      FMT_STRING("{}"),
      _nonce.count());
  _sha.clear();
  _sha.update(nonce);
  _sha.update(body);
  std::array<char, 32> buffer_1;
  auto length_1 = _sha.digest(
      buffer_1.data(),
      buffer_1.size());
  assert(length_1 == buffer_1.size());
  _hmac.clear();
  _hmac.update(path.data(), path.length());
  _hmac.update(buffer_1.data(), buffer_1.size());
  std::array<char, 64> buffer_2;
  auto length_2 = _hmac.digest(
      buffer_2.data(),
      buffer_2.size());
  assert(length_2 == buffer_2.size());
  auto sign_2 = core::binascii::Base64::encode(
      buffer_2.data(),
      length_2);
  return fmt::format(
      FMT_STRING(
        "API-Key: {}\r\n"
        "API-Sign: {}\r\n"),
      _key,
      sign_2);
}

}  // namespace kraken
}  // namespace roq

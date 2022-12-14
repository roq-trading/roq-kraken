/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/kraken/tools/hasher.hpp"

#include <random>
#include <vector>

#include "roq/logging.hpp"

#include "roq/clock.hpp"

#include "roq/core/codec/base64.hpp"

using namespace std::literals;
using namespace std::literals;

namespace roq {
namespace kraken {
namespace tools {

// === CONSTANTS ===

namespace {
constexpr auto const THRESHOLD = -1000ms;
}

// === CONSTANTS ===

namespace {
template <typename R>
R create_hmac(auto const &secret) {
  std::vector<std::byte> buffer;
  buffer.resize(core::codec::Base64::get_max_binary_length(std::size(secret)));
  auto raw_secret = core::codec::Base64::decode(buffer, secret, false, false);
  return R{raw_secret};
}
}  // namespace

Hasher::Hasher(std::string_view const &key, std::string_view const &secret, std::string_view const &passphrase)
    : key_{key}, passphrase_{passphrase}, mac_{create_hmac<decltype(mac_)>(secret)} {
}

std::string Hasher::create_body() {
  auto now = std::chrono::duration_cast<decltype(nonce_)>(clock::get_realtime());
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
  hash_.clear();
  hash_.update(nonce);
  hash_.update(body);
  std::array<std::byte, Hash::DIGEST_LENGTH> buffer_1;
  auto digest_1 = hash_.final(buffer_1);
  mac_.clear();
  mac_.update(path);
  mac_.update(digest_1);
  auto digest_2 = mac_.final(digest_);
  std::string sign_2;
  core::codec::Base64::encode(sign_2, digest_2, false, false);
  return fmt::format(
      "API-Key: {}\r\n"
      "API-Sign: {}\r\n"sv,
      key_,
      sign_2);
}

}  // namespace tools
}  // namespace kraken
}  // namespace roq

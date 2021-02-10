/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/flags.h"

#include <absl/flags/declare.h>
#include <absl/flags/flag.h>

#include <string>

#include "roq/core/flags/non_empty.h"
#include "roq/core/flags/non_zero.h"
#include "roq/core/flags/path.h"
#include "roq/core/flags/uri.h"

using namespace std::literals;  // NOLINT

ABSL_FLAG(  //
    roq::core::flags::Path<std::string>,
    config_file,
    {},
    "config file (path)"sv);

ABSL_FLAG(  //
    roq::core::flags::NonEmpty<std::string>,
    exchange,
    "kraken"s,
    "exchange identifier (string)"sv);

// rest

ABSL_FLAG(  //
    roq::core::flags::URI<std::string>,
    rest_uri,
    "https://api.kraken.com"s,
    "REST end-point (URI)"sv);

ABSL_FLAG(  //
    roq::core::flags::NonZero<uint32_t>,
    rest_ping_freq_secs,
    uint32_t{5},
    "ping frequency (seconds)"sv);

ABSL_FLAG(  //
    roq::core::flags::Path<std::string>,
    rest_ping_path,
    "/0/public/Time"s,
    "URI path used for REST connection keep-alive messages"sv);

ABSL_FLAG(  //
    uint32_t,
    rest_request_queue_depth,
    uint32_t{5},
    "request: max queue depth"sv);

ABSL_FLAG(  //
    uint32_t,
    rest_request_timeout_secs,
    uint32_t{30},
    "request: timeout (seconds)"sv);

ABSL_FLAG(  //
    uint32_t,
    rest_rate_limit_interval_secs,
    uint32_t{1},
    "rate limit: monitor interval (seconds)"sv);

ABSL_FLAG(  //
    uint32_t,
    rest_rate_limit_max_requests,
    uint32_t{300},
    "rate limit: max requests (per interval)"sv);

// ws public

ABSL_FLAG(  //
    roq::core::flags::URI<std::string>,
    ws_public_uri,
    "wss://beta-ws.kraken.com"s,
    "WebSocket end-point (URI)"sv);

ABSL_FLAG(  //
    uint32_t,
    ws_public_ping_freq_secs,
    uint32_t{5},
    "ping frequency (seconds)"sv);

ABSL_FLAG(  //
    uint32_t,
    ws_public_request_timeout_secs,
    uint32_t{15},
    "request time-out (seconds)"sv);

ABSL_FLAG(  //
    uint32_t,
    ws_public_subscribe_book_depth,
    uint32_t{10},
    "book depth"sv);

// ws private

ABSL_FLAG(  //
    roq::core::flags::URI<std::string>,
    ws_private_uri,
    "wss://beta-ws-auth.kraken.com"s,
    "WebSocket end-point (URI)"sv);

ABSL_FLAG(  //
    uint32_t,
    ws_private_ping_freq_secs,
    uint32_t{5},
    "ping frequency (seconds)"sv);

ABSL_FLAG(  //
    uint32_t,
    ws_private_request_timeout_secs,
    uint32_t{15},
    "request time-out (seconds)"sv);

// XXX review

ABSL_FLAG(  //
    roq::core::flags::NonZero<uint32_t>,
    encode_buffer_size,
    uint32_t{1048576},
    "encode buffer size"sv);

ABSL_FLAG(  //
    roq::core::flags::NonZero<uint32_t>,
    decode_buffer_size,
    uint32_t{10485760},
    "decode buffer size"sv);

// external

ABSL_DECLARE_FLAG(roq::core::flags::NonEmpty<std::string>, name);
ABSL_DECLARE_FLAG(roq::core::flags::NonZero<uint32_t>, cache_mbp_max_depth);
ABSL_DECLARE_FLAG(roq::core::flags::NonZero<uint32_t>, cache_trades_max_depth);

namespace roq {
namespace kraken {

std::string_view Flags::config_file() {
  static const std::string result = absl::GetFlag(FLAGS_config_file);
  return result;
}

std::string_view Flags::exchange() {
  static const std::string result = absl::GetFlag(FLAGS_exchange);
  return result;
}

std::string_view Flags::rest_uri() {
  static const std::string result = absl::GetFlag(FLAGS_rest_uri);
  return result;
}

uint32_t Flags::rest_ping_freq_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_rest_ping_freq_secs);
  return result;
}

std::string_view Flags::rest_ping_path() {
  static const std::string result = absl::GetFlag(FLAGS_rest_ping_path);
  return result;
}

uint32_t Flags::rest_request_queue_depth() {
  static const uint32_t result = absl::GetFlag(FLAGS_rest_request_queue_depth);
  return result;
}

uint32_t Flags::rest_request_timeout_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_rest_request_timeout_secs);
  return result;
}

uint32_t Flags::rest_rate_limit_interval_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_rest_rate_limit_interval_secs);
  return result;
}

uint32_t Flags::rest_rate_limit_max_requests() {
  static const uint32_t result = absl::GetFlag(FLAGS_rest_rate_limit_max_requests);
  return result;
}

std::string_view Flags::ws_public_uri() {
  static const std::string result = absl::GetFlag(FLAGS_ws_public_uri);
  return result;
}

uint32_t Flags::ws_public_ping_freq_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_ws_public_ping_freq_secs);
  return result;
}

uint32_t Flags::ws_public_request_timeout_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_ws_public_request_timeout_secs);
  return result;
}

uint32_t Flags::ws_public_subscribe_book_depth() {
  static const uint32_t result = absl::GetFlag(FLAGS_ws_public_subscribe_book_depth);
  return result;
}

std::string_view Flags::ws_private_uri() {
  static const std::string result = absl::GetFlag(FLAGS_ws_private_uri);
  return result;
}

uint32_t Flags::ws_private_ping_freq_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_ws_private_ping_freq_secs);
  return result;
}

uint32_t Flags::ws_private_request_timeout_secs() {
  static const uint32_t result = absl::GetFlag(FLAGS_ws_private_request_timeout_secs);
  return result;
}

uint32_t Flags::encode_buffer_size() {
  static const uint32_t result = absl::GetFlag(FLAGS_encode_buffer_size);
  return result;
}

uint32_t Flags::decode_buffer_size() {
  static const uint32_t result = absl::GetFlag(FLAGS_decode_buffer_size);
  return result;
}

std::string_view Flags::name() {
  static const std::string result = absl::GetFlag(FLAGS_name);
  return result;
}

uint32_t Flags::cache_mbp_max_depth() {
  static const uint32_t result = absl::GetFlag(FLAGS_cache_mbp_max_depth);
  return result;
}

uint32_t Flags::cache_trades_max_depth() {
  static const uint32_t result = absl::GetFlag(FLAGS_cache_trades_max_depth);
  return result;
}

}  // namespace kraken
}  // namespace roq

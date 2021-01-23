/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <cstdint>
#include <string_view>

namespace roq {
namespace kraken {

struct Flags final {
  static std::string_view config_file();
  static std::string_view exchange();
  static std::string_view rest_uri();
  static uint32_t rest_ping_freq_secs();
  static std::string_view rest_ping_path();
  static uint32_t rest_request_queue_depth();
  static uint32_t rest_request_timeout_secs();
  static uint32_t rest_rate_limit_interval_secs();
  static uint32_t rest_rate_limit_max_requests();
  static std::string_view ws_public_uri();
  static uint32_t ws_public_ping_freq_secs();
  static uint32_t ws_public_request_timeout_secs();
  static uint32_t ws_public_subscribe_book_depth();
  static std::string_view ws_private_uri();
  static uint32_t ws_private_ping_freq_secs();
  static uint32_t ws_private_request_timeout_secs();
  static uint32_t encode_buffer_size();
  static uint32_t decode_buffer_size();
  // external
  static std::string_view name();
  static uint32_t cache_mbp_max_depth();
  static uint32_t cache_trades_max_depth();
};

}  // namespace kraken
}  // namespace roq

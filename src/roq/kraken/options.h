/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <absl/flags/declare.h>

#include <cstdint>
#include <string>

ABSL_DECLARE_FLAG(std::string, config_file);

ABSL_DECLARE_FLAG(std::string, exchange);

// rest
ABSL_DECLARE_FLAG(std::string, rest_uri);
ABSL_DECLARE_FLAG(uint32_t, rest_ping_freq_secs);
ABSL_DECLARE_FLAG(std::string, rest_ping_path);
ABSL_DECLARE_FLAG(uint32_t, rest_request_queue_depth);
ABSL_DECLARE_FLAG(uint32_t, rest_request_timeout_secs);
ABSL_DECLARE_FLAG(uint32_t, rest_rate_limit_interval_secs);
ABSL_DECLARE_FLAG(uint32_t, rest_rate_limit_max_requests);

// ws public
ABSL_DECLARE_FLAG(std::string, ws_public_uri);
ABSL_DECLARE_FLAG(uint32_t, ws_public_ping_freq_secs);
ABSL_DECLARE_FLAG(uint32_t, ws_public_request_timeout_secs);
ABSL_DECLARE_FLAG(uint32_t, ws_public_subscribe_book_depth);

// ws private
ABSL_DECLARE_FLAG(std::string, ws_private_uri);
ABSL_DECLARE_FLAG(uint32_t, ws_private_ping_freq_secs);
ABSL_DECLARE_FLAG(uint32_t, ws_private_request_timeout_secs);

// XXX review
ABSL_DECLARE_FLAG(uint32_t, encode_buffer_size);
ABSL_DECLARE_FLAG(uint32_t, decode_buffer_size);

// external
ABSL_DECLARE_FLAG(std::string, name);
ABSL_DECLARE_FLAG(uint32_t, cache_mbp_max_depth);
ABSL_DECLARE_FLAG(uint32_t, cache_trades_max_depth);

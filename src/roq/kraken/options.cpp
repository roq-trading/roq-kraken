/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/options.h"

#include <absl/flags/flag.h>

ABSL_FLAG(std::string, config_file, "", "config file (path)");

ABSL_FLAG(std::string, exchange, "kraken", "exchange identifier (string)");

// rest

ABSL_FLAG(
    std::string, rest_uri, "https://api.kraken.com", "REST end-point (URI)");

ABSL_FLAG(uint32_t, rest_ping_freq_secs, 5, "ping frequency (seconds)");

ABSL_FLAG(
    std::string,
    rest_ping_path,
    "/0/public/Time",
    "URI path used for REST connection keep-alive messages");

ABSL_FLAG(uint32_t, rest_request_queue_depth, 5, "request: max queue depth");

ABSL_FLAG(
    uint32_t, rest_request_timeout_secs, 30, "request: timeout (seconds)");

ABSL_FLAG(
    uint32_t,
    rest_rate_limit_interval_secs,
    1,
    "rate limit: monitor interval (seconds)");

ABSL_FLAG(
    uint32_t,
    rest_rate_limit_max_requests,
    300,
    "rate limit: max requests (per interval)");

// ws public

ABSL_FLAG(
    std::string,
    ws_public_uri,
    "wss://beta-ws.kraken.com",
    "WebSocket end-point (URI)");

ABSL_FLAG(uint32_t, ws_public_ping_freq_secs, 5, "ping frequency (seconds)");

ABSL_FLAG(
    uint32_t, ws_public_request_timeout_secs, 15, "request time-out (seconds)");

ABSL_FLAG(
    uint32_t,
    ws_public_subscribe_book_depth,
    0,
    "book depth (0 == exchange default)");

// ws private

ABSL_FLAG(
    std::string,
    ws_private_uri,
    "wss://beta-ws-auth.kraken.com",
    "WebSocket end-point (URI)");

ABSL_FLAG(uint32_t, ws_private_ping_freq_secs, 5, "ping frequency (seconds)");

ABSL_FLAG(
    uint32_t,
    ws_private_request_timeout_secs,
    15,
    "request time-out (seconds)");

// XXX review

ABSL_FLAG(uint32_t, encode_buffer_size, 1048576, "encode buffer size");

ABSL_FLAG(uint32_t, decode_buffer_size, 10485760, "decode buffer size");

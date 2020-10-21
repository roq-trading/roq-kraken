/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/options.h"

DEFINE_string(config_file, "", "config file (path)");

DEFINE_string(exchange, "kraken", "exchange identifier (string)");

DEFINE_uint32(download_timeout_secs, 15, "download time-out (seconds)");

DEFINE_string(rest_uri, "https://api.kraken.com", "REST end-point (URI)");

DEFINE_uint32(rest_ping_freq_secs, 5, "ping frequency (seconds)");

DEFINE_string(
    rest_ping_path,
    "/0/public/Time",
    "URI path used for REST connection keep-alive messages");

DEFINE_uint32(rest_request_queue_depth, 5, "request: max queue depth");

DEFINE_uint32(rest_request_timeout_secs, 30, "request: timeout (seconds)");

DEFINE_uint32(
    rest_rate_limit_interval_secs, 1, "rate limit: monitor interval (seconds)");

DEFINE_uint32(
    rest_rate_limit_max_requests,
    300,
    "rate limit: max requests (per interval)");

DEFINE_string(
    ws_public_uri, "wss://beta-ws.kraken.com", "WebSocket end-point (URI)");

DEFINE_uint32(ws_public_ping_freq_secs, 5, "ping frequency (seconds)");

DEFINE_uint32(ws_public_book_depth, 0, "book depth (0 == exchange default)");

DEFINE_string(
    ws_private_uri,
    "wss://beta-ws-auth.kraken.com",
    "WebSocket end-point (URI)");

DEFINE_uint32(ws_private_ping_freq_secs, 5, "ping frequency (seconds)");

DEFINE_uint32(encode_buffer_size, 1048576, "encode buffer size");

DEFINE_uint32(decode_buffer_size, 10485760, "decode buffer size");

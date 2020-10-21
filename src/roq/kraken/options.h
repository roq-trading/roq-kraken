/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <gflags/gflags.h>

DECLARE_string(config_file);

DECLARE_string(exchange);

DECLARE_uint32(download_timeout_secs);

DECLARE_string(rest_uri);
DECLARE_uint32(rest_ping_freq_secs);
DECLARE_string(rest_ping_path);
DECLARE_uint32(rest_request_queue_depth);
DECLARE_uint32(rest_request_timeout_secs);
DECLARE_uint32(rest_rate_limit_interval_secs);
DECLARE_uint32(rest_rate_limit_max_requests);

DECLARE_string(ws_public_uri);
DECLARE_uint32(ws_public_ping_freq_secs);
DECLARE_uint32(ws_public_book_depth);

DECLARE_string(ws_private_uri);
DECLARE_uint32(ws_private_ping_freq_secs);

// XXX review

DECLARE_uint32(encode_buffer_size);
DECLARE_uint32(decode_buffer_size);

// external

DECLARE_string(name);
DECLARE_uint32(cache_mbp_max_depth);
DECLARE_uint32(cache_trades_max_depth);

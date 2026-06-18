/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/gateway/shared.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace gateway {

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : dispatcher{dispatcher}, settings{settings}, api{API::create(settings)}, symbols{settings.ws.public_max_subscriptions_per_stream} {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  request_id_encode_buffer_.clear();
  fmt::format_to(std::back_inserter(request_id_encode_buffer_), "roq-{}"sv, request_id);
  return request_id_encode_buffer_;
}

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

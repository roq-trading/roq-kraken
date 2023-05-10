/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/kraken/shared.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === IMPLEMENTATION ===

Shared::Shared(server::Dispatcher &dispatcher, Settings const &settings)
    : dispatcher_{dispatcher}, settings{settings}, symbols{settings.ws.public_max_subscriptions_per_stream} {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  stack_buffer_.clear();
  fmt::format_to(std::back_inserter(stack_buffer_), "roq-{}"sv, request_id);
  return {std::data(stack_buffer_), std::size(stack_buffer_)};
}

}  // namespace kraken
}  // namespace roq

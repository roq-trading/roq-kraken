/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/shared.hpp"

#include "roq/kraken/flags.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

Shared::Shared(server::Dispatcher &dispatcher)
    : bids(server::Flags::cache_mbp_max_depth()), asks(server::Flags::cache_mbp_max_depth()),
      final_bids(server::Flags::cache_mbp_max_depth()),
      final_asks(server::Flags::cache_mbp_max_depth()),
      trades(server::Flags::cache_trades_max_depth()), dispatcher_(dispatcher),
      symbols(Flags::ws_public_max_subscriptions_per_stream()) {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  stack_buffer_.clear();
  fmt::format_to(std::back_inserter(stack_buffer_), "roq-{}"sv, request_id);
  return std::string_view{std::data(stack_buffer_), std::size(stack_buffer_)};
}

}  // namespace kraken
}  // namespace roq

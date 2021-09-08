/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/shared.h"

#include "roq/kraken/flags.h"

namespace roq {
namespace kraken {

Shared::Shared(server::Dispatcher &dispatcher)
    : bids(server::Flags::cache_mbp_max_depth()), asks(server::Flags::cache_mbp_max_depth()),
      final_bids(server::Flags::cache_mbp_max_depth()),
      final_asks(server::Flags::cache_mbp_max_depth()),
      trades(server::Flags::cache_trades_max_depth()), dispatcher_(dispatcher) {
}

std::string_view Shared::next_request_id() {
  auto request_id = ++request_id_;
  stack_buffer_.clear();
  fmt::format_to(std::back_inserter(stack_buffer_), "roq-{}"_sv, request_id);
  return std::string_view{stack_buffer_.data(), stack_buffer_.size()};
}

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/json/balances.hpp"
#include "roq/kraken/json/book.hpp"
#include "roq/kraken/json/error_2.hpp"
#include "roq/kraken/json/executions.hpp"
#include "roq/kraken/json/heartbeat_2.hpp"
#include "roq/kraken/json/instrument.hpp"
#include "roq/kraken/json/pong_2.hpp"
#include "roq/kraken/json/status.hpp"
#include "roq/kraken/json/subscribe.hpp"
#include "roq/kraken/json/ticker.hpp"
#include "roq/kraken/json/trade.hpp"

namespace roq {
namespace kraken {
namespace json {

struct Parser final {
  struct Handler {
    virtual void operator()(Trace<Status> const &) = 0;
    virtual void operator()(Trace<Heartbeat2> const &) = 0;

    virtual void operator()(Trace<Error2> const &) = 0;
    virtual void operator()(Trace<Pong2> const &) = 0;
    virtual void operator()(Trace<Subscribe> const &) = 0;

    virtual void operator()(Trace<Instrument> const &) = 0;

    virtual void operator()(Trace<Ticker> const &) = 0;
    virtual void operator()(Trace<Trade> const &) = 0;
    virtual void operator()(Trace<Book> const &) = 0;

    virtual void operator()(Trace<Balances> const &) = 0;
    virtual void operator()(Trace<Executions> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

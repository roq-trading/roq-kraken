/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/json/add_order.hpp"
#include "roq/kraken/json/balances.hpp"
#include "roq/kraken/json/book.hpp"
#include "roq/kraken/json/cancel_all.hpp"
#include "roq/kraken/json/cancel_order.hpp"
#include "roq/kraken/json/error.hpp"
#include "roq/kraken/json/executions.hpp"
#include "roq/kraken/json/heartbeat.hpp"
#include "roq/kraken/json/instrument.hpp"
#include "roq/kraken/json/pong.hpp"
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
    virtual void operator()(Trace<Heartbeat> const &) = 0;

    virtual void operator()(Trace<Error> const &) = 0;
    virtual void operator()(Trace<Pong> const &) = 0;
    virtual void operator()(Trace<Subscribe> const &) = 0;

    virtual void operator()(Trace<Instrument> const &) = 0;

    virtual void operator()(Trace<Ticker> const &) = 0;
    virtual void operator()(Trace<Trade> const &) = 0;
    virtual void operator()(Trace<Book> const &) = 0;

    virtual void operator()(Trace<Balances> const &) = 0;
    virtual void operator()(Trace<Executions> const &) = 0;

    virtual void operator()(Trace<AddOrder> const &) = 0;
    virtual void operator()(Trace<CancelOrder> const &) = 0;
    virtual void operator()(Trace<CancelAll> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

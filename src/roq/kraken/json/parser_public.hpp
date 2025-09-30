/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/trace_info.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/json/error.hpp"
#include "roq/kraken/json/heartbeat.hpp"
#include "roq/kraken/json/pong.hpp"
#include "roq/kraken/json/subscription_status.hpp"
#include "roq/kraken/json/system_status.hpp"

#include "roq/kraken/json/book.hpp"
#include "roq/kraken/json/spread.hpp"
#include "roq/kraken/json/trade.hpp"

namespace roq {
namespace kraken {
namespace json {

struct ParserPublic final {
  struct Handler {
    virtual void operator()(Trace<Error> const &) = 0;
    virtual void operator()(Trace<SystemStatus> const &) = 0;
    virtual void operator()(Trace<Pong> const &) = 0;
    virtual void operator()(Trace<Heartbeat> const &) = 0;
    virtual void operator()(Trace<SubscriptionStatus> const &) = 0;

    virtual void operator()(Trace<Trade> const &, std::string_view const &pair) = 0;
    virtual void operator()(Trace<Spread> const &, std::string_view const &pair) = 0;
    virtual void operator()(Trace<Book> const &, std::string_view const &pair) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, TraceInfo const &, bool allow_unknown_event_types);

 protected:
  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, core::json::Object &root, TraceInfo const &);

  static bool dispatch(Handler &, std::string_view const &message, core::json::BufferStack &, core::json::Array &root, TraceInfo const &trace_info);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/parser.hpp"

#include "roq/server.hpp"

#include "roq/kraken/json/error.hpp"
#include "roq/kraken/json/heartbeat.hpp"
#include "roq/kraken/json/pong.hpp"
#include "roq/kraken/json/subscription_status.hpp"
#include "roq/kraken/json/system_status.hpp"

#include "roq/kraken/json/add_order_status.hpp"
#include "roq/kraken/json/cancel_order_status.hpp"

#include "roq/kraken/json/open_orders.hpp"
#include "roq/kraken/json/own_trades.hpp"

namespace roq {
namespace kraken {
namespace json {

struct ParserPrivate final {
  struct Handler {
    virtual void operator()(Trace<Error const> const &) = 0;
    virtual void operator()(Trace<SystemStatus const> const &) = 0;
    virtual void operator()(Trace<Pong const> const &) = 0;
    virtual void operator()(Trace<Heartbeat const> const &) = 0;
    virtual void operator()(Trace<SubscriptionStatus const> const &) = 0;

    virtual void operator()(Trace<AddOrderStatus const> const &) = 0;
    virtual void operator()(Trace<CancelOrderStatus const> const &) = 0;

    virtual void operator()(Trace<OpenOrders const> const &) = 0;
    virtual void operator()(Trace<OwnTrades const> const &) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::Buffer &, TraceInfo const &);

 protected:
  static bool dispatch(
      Handler &, std::string_view const &message, core::json::Buffer &, core::json::Object &root, TraceInfo const &);

  static bool dispatch(
      Handler &, std::string_view const &message, core::json::Buffer &, core::json::Array &root, TraceInfo const &);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

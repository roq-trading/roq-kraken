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
    virtual void operator()(const Trace<Error const> &) = 0;
    virtual void operator()(const Trace<SystemStatus const> &) = 0;
    virtual void operator()(const Trace<Pong const> &) = 0;
    virtual void operator()(const Trace<Heartbeat const> &) = 0;
    virtual void operator()(const Trace<SubscriptionStatus const> &) = 0;

    virtual void operator()(const Trace<AddOrderStatus const> &) = 0;
    virtual void operator()(const Trace<CancelOrderStatus const> &) = 0;

    virtual void operator()(const Trace<OpenOrders const> &) = 0;
    virtual void operator()(const Trace<OwnTrades const> &) = 0;
  };

  static bool dispatch(
      Handler &, const std::string_view &message, core::json::Buffer &, const TraceInfo &);

 protected:
  static bool dispatch(
      Handler &,
      const std::string_view &message,
      core::json::Buffer &,
      core::json::object_t &root,
      const TraceInfo &);

  static bool dispatch(
      Handler &,
      const std::string_view &message,
      core::json::Buffer &,
      core::json::array_t &root,
      const TraceInfo &);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

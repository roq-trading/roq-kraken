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
    virtual void operator()(const Trace<Error> &) = 0;
    virtual void operator()(const Trace<SystemStatus> &) = 0;
    virtual void operator()(const Trace<Pong> &) = 0;
    virtual void operator()(const Trace<Heartbeat> &) = 0;
    virtual void operator()(const Trace<SubscriptionStatus> &) = 0;

    virtual void operator()(const Trace<AddOrderStatus> &) = 0;
    virtual void operator()(const Trace<CancelOrderStatus> &) = 0;

    virtual void operator()(const Trace<OpenOrders> &) = 0;
    virtual void operator()(const Trace<OwnTrades> &) = 0;
  };

  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const TraceInfo &trace_info);

 protected:
  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      core::json::object_t &root,
      const TraceInfo &trace_info);

  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      core::json::array_t &root,
      const TraceInfo &trace_info);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

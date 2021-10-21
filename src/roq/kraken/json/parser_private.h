/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/parser.h"

#include "roq/server.h"

#include "roq/kraken/json/error.h"
#include "roq/kraken/json/heartbeat.h"
#include "roq/kraken/json/pong.h"
#include "roq/kraken/json/subscription_status.h"
#include "roq/kraken/json/system_status.h"

#include "roq/kraken/json/add_order_status.h"
#include "roq/kraken/json/cancel_order_status.h"

#include "roq/kraken/json/open_orders.h"
#include "roq/kraken/json/own_trades.h"

namespace roq {
namespace kraken {
namespace json {

struct ParserPrivate final {
  struct Handler {
    virtual void operator()(const server::Trace<Error> &) = 0;
    virtual void operator()(const server::Trace<SystemStatus> &) = 0;
    virtual void operator()(const server::Trace<Pong> &) = 0;
    virtual void operator()(const server::Trace<Heartbeat> &) = 0;
    virtual void operator()(const server::Trace<SubscriptionStatus> &) = 0;

    virtual void operator()(const server::Trace<AddOrderStatus> &) = 0;
    virtual void operator()(const server::Trace<CancelOrderStatus> &) = 0;

    virtual void operator()(const server::Trace<OpenOrders> &) = 0;
    virtual void operator()(const server::Trace<OwnTrades> &) = 0;
  };

  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      const server::TraceInfo &trace_info);

 protected:
  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      core::json::object_t &root,
      const server::TraceInfo &trace_info);

  static bool dispatch(
      Handler &handler,
      const std::string_view &message,
      core::json::Buffer &buffer,
      core::json::array_t &root,
      const server::TraceInfo &trace_info);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

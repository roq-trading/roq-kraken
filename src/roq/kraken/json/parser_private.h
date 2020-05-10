/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/parser.h"

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
    virtual void operator()(const Error&) = 0;
    virtual void operator()(const SystemStatus&) = 0;
    virtual void operator()(const Pong&) = 0;
    virtual void operator()(const Heartbeat&) = 0;
    virtual void operator()(const SubscriptionStatus&) = 0;

    virtual void operator()(const AddOrderStatus&) = 0;
    virtual void operator()(const CancelOrderStatus&) = 0;

    virtual void operator()(const OpenOrders&) = 0;
    virtual void operator()(const OwnTrades&) = 0;
  };

  static bool dispatch(
      Handler& handler,
      const std::string_view& message,
      core::json::Buffer& buffer);

 protected:
  static bool dispatch(
      Handler& handler,
      const std::string_view& message,
      core::json::Buffer& buffer,
      core::json::object_t& root);

  static bool dispatch(
      Handler& handler,
      const std::string_view& message,
      core::json::Buffer& buffer,
      core::json::array_t& root);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

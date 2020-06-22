/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/json/parser.h"

#include "roq/server.h"

#include "roq/kraken/json/error.h"
#include "roq/kraken/json/heartbeat.h"
#include "roq/kraken/json/pong.h"
#include "roq/kraken/json/subscription_status.h"
#include "roq/kraken/json/system_status.h"

#include "roq/kraken/json/book.h"
#include "roq/kraken/json/spread.h"
#include "roq/kraken/json/trade.h"

namespace roq {
namespace kraken {
namespace json {

struct ParserPublic final {
  struct Handler {
    virtual void operator()(
        const Error&,
        const server::Trace&) = 0;
    virtual void operator()(
        const SystemStatus&,
        const server::Trace&) = 0;
    virtual void operator()(
        const Pong&,
        const server::Trace&) = 0;
    virtual void operator()(
        const Heartbeat&,
        const server::Trace&) = 0;
    virtual void operator()(
        const SubscriptionStatus&,
        const server::Trace&) = 0;

    virtual void operator()(
        const Trade& trade,
        const std::string_view& pair,
        const server::Trace& trace) = 0;
    virtual void operator()(
        const Spread& spread,
        const std::string_view& pair,
        const server::Trace& trace) = 0;
    virtual void operator()(
        const Book& book,
        const std::string_view& pair,
        const server::Trace& trace) = 0;
  };

  static bool dispatch(
      Handler& handler,
      const std::string_view& message,
      core::json::Buffer& buffer,
      const server::Trace& trace);

 protected:
  static bool dispatch(
      Handler& handler,
      const std::string_view& message,
      core::json::Buffer& buffer,
      core::json::object_t& root,
      const server::Trace& trace);

  static bool dispatch(
      Handler& handler,
      const std::string_view& message,
      core::json::Buffer& buffer,
      core::json::array_t& root,
      const server::Trace& trace);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

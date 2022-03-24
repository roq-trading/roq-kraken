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

#include "roq/kraken/json/book.hpp"
#include "roq/kraken/json/spread.hpp"
#include "roq/kraken/json/trade.hpp"

namespace roq {
namespace kraken {
namespace json {

struct ParserPublic final {
  struct Handler {
    virtual void operator()(const Trace<Error> &) = 0;
    virtual void operator()(const Trace<SystemStatus> &) = 0;
    virtual void operator()(const Trace<Pong> &) = 0;
    virtual void operator()(const Trace<Heartbeat> &) = 0;
    virtual void operator()(const Trace<SubscriptionStatus> &) = 0;

    virtual void operator()(const Trace<Trade> &, const std::string_view &pair) = 0;
    virtual void operator()(const Trace<Spread> &, const std::string_view &pair) = 0;
    virtual void operator()(const Trace<Book> &, const std::string_view &pair) = 0;
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

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
    virtual void operator()(Trace<Error const> const &) = 0;
    virtual void operator()(Trace<SystemStatus const> const &) = 0;
    virtual void operator()(Trace<Pong const> const &) = 0;
    virtual void operator()(Trace<Heartbeat const> const &) = 0;
    virtual void operator()(Trace<SubscriptionStatus const> const &) = 0;

    virtual void operator()(Trace<Trade const> const &, std::string_view const &pair) = 0;
    virtual void operator()(Trace<Spread const> const &, std::string_view const &pair) = 0;
    virtual void operator()(Trace<Book const> const &, std::string_view const &pair) = 0;
  };

  static bool dispatch(Handler &, std::string_view const &message, core::json::Buffer &, TraceInfo const &);

 protected:
  static bool dispatch(
      Handler &, std::string_view const &message, core::json::Buffer &, core::json::Object &root, TraceInfo const &);

  static bool dispatch(
      Handler &,
      std::string_view const &message,
      core::json::Buffer &,
      core::json::Array &root,
      TraceInfo const &trace_info);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/json/parser.h"

#include "roq/logging.h"

#include "roq/kraken/json/channel.h"
#include "roq/kraken/json/event.h"
#include "roq/kraken/json/result_field.h"

namespace roq {
namespace kraken {
namespace json {

bool Parser::dispatch(
    Handler& handler,
    const std::string_view& message,
    core::json::Buffer& buffer) {
  // different parsing depending on object or array representation
  core::json::Parser parser(message);
  auto root = parser.root();
  return std::visit(
      overloaded {
        [](const core::json::null_t&) -> bool {
          throw std::bad_cast();
        },
        [](bool) -> bool {
          throw std::bad_cast();
        },
        [](int64_t) -> bool {
          throw std::bad_cast();
        },
        [](double) -> bool {
          throw std::bad_cast();
        },
        [](const std::string_view&) -> bool {
          throw std::bad_cast();
        },
        [&](core::json::object_t& value) -> bool {
          return dispatch(handler, message, buffer, value);
        },
        [&](core::json::array_t& value) -> bool {
          return dispatch(handler, message, buffer, value);
        },
      },
      root);
}

bool Parser::dispatch(
    Handler& handler,
    const std::string_view& message,
    core::json::Buffer& buffer,
    core::json::object_t& root) {
  bool dispatched = false;
  for (auto [key, value] : root) {
    auto field = ResultField(key);
    switch (field) {
      case ResultField::UNDEFINED:
      case ResultField::UNKNOWN:
        break;
      case ResultField::EVENT: {
        auto event = Event(value);
        switch (event) {
          case Event::UNDEFINED:
            LOG(FATAL)("Unexpected");
            break;
          case Event::UNKNOWN:
            DLOG(FATAL)(
                FMT_STRING("Unknown key=\"{}\""),
                key);
            break;
          case Event::ERROR: {
            auto error = core::json::Parser::create<Error>(message);
            handler(error);
            dispatched = true;
            break;
          }
          case Event::SYSTEM_STATUS: {
            auto system_status =
              core::json::Parser::create<SystemStatus>(message);
            handler(system_status);
            dispatched = true;
            break;
          }
          case Event::PONG: {
            auto pong = core::json::Parser::create<Pong>(message);
            handler(pong);
            dispatched = true;
            break;
          }
          case Event::HEARTBEAT: {
            auto heartbeat =
              core::json::Parser::create<Heartbeat>(message);
            handler(heartbeat);
            dispatched = true;
            break;
          }
          case Event::SUBSCRIPTION_STATUS: {
            auto subscription_status =
              core::json::Parser::create<SubscriptionStatus>(message);
            handler(subscription_status);
            dispatched = true;
            break;
          }
          case Event::ADD_ORDER_STATUS: {
            throw std::runtime_error("addOrderStatus not supported");
            break;
          }
          case Event::CANCEL_ORDER_STATUS:
            throw std::runtime_error("cancelOrderStatus not supported");
            break;
        }
        break;
      }
    }
  }
  return dispatched;
}

namespace {
static bool dispatch2(
    Parser::Handler& handler,
    const std::string_view& message,
    core::json::Buffer& buffer,
    int64_t channel_id,
    Channel channel,
    const std::string_view& pair) {
  DLOG(INFO)(
      FMT_STRING(R"(channel_id={} channel={} pair={})"),
      channel_id,
      channel,
      pair);
  bool dispatched = false;
  core::json::Parser parser(message);
  auto root = parser.root();
  size_t offset = 0;
  for (auto value : std::get<core::json::array_t>(root)) {
    if (++offset != 2)
      continue;
    switch (channel) {
      case Channel::UNDEFINED:
      case Channel::UNKNOWN:
        LOG(FATAL)("Unexpected");
        break;
      case Channel::TICKER: {
        throw std::runtime_error("ticker not supported");
        break;
      }
      case Channel::OHLC: {
        throw std::runtime_error("ohlc not supported");
        break;
      }
      case Channel::TRADE: {
        Trade trade(
            value,
            buffer);
        handler(trade);
        dispatched = true;
        break;
      }
      case Channel::SPREAD: {
        Spread spread(value);
        handler(spread);
        dispatched = true;
        break;
      }
      case Channel::BOOK: {
        Book book(
            value,
            buffer);
        handler(book);
        dispatched = true;
        break;
      }
      case Channel::OWN_TRADES: {
        throw std::runtime_error("ownTrades not supported");
        break;
      }
      case Channel::OPEN_ORDERS: {
        throw std::runtime_error("openOrders not supported");
        break;
      }
    }
  }
  return dispatched;
}
}  // namespace

bool Parser::dispatch(
    Handler& handler,
    const std::string_view& message,
    core::json::Buffer& buffer,
    core::json::array_t& root) {
  int64_t channel_id = 0;
  Channel channel = Channel::UNDEFINED;
  std::string_view pair;
  size_t offset = 0;
  for (auto value : root) {
    switch (++offset) {
      case 1:
        channel_id = std::get<int64_t>(value);
        break;
      case 2:
        // we need a second pass to parse this...
        break;
      case 3: {
        auto name = std::get<std::string_view>(value);
        auto pos = name.find_first_of('-');
        if (pos != name.npos)
          name.remove_suffix(name.size() - pos);
        channel = Channel(name);
        DLOG_IF(FATAL, channel == Channel::UNKNOWN)(
            FMT_STRING(R"(Unknown channel="{}")"),
            name);
        break;
      }
      case 4:
        pair = std::get<std::string_view>(value);
        break;
      default:
        break;
    }
  }
  LOG_IF(FATAL, offset != 4)(
      FMT_STRING(R"(Recived {} columns, expected 4)"),
      offset);
  return dispatch2(
      handler,
      message,
      buffer,
      channel_id,
      channel,
      pair);
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

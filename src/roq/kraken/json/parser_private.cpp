/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/json/parser_private.h"

#include "roq/logging.h"

#include "roq/kraken/json/channel.h"
#include "roq/kraken/json/event.h"
#include "roq/kraken/json/result_field.h"

namespace roq {
namespace kraken {
namespace json {

bool ParserPrivate::dispatch(
    Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info) {
  // different parsing depending on object or array representation
  core::json::Parser parser(message);
  auto root = parser.root();
  return std::visit(
      overloaded{
          [](const core::json::null_t &) -> bool { throw std::bad_cast(); },
          [](bool) -> bool { throw std::bad_cast(); },
          [](int64_t) -> bool { throw std::bad_cast(); },
          [](double) -> bool { throw std::bad_cast(); },
          [](const std::string_view &) -> bool { throw std::bad_cast(); },
          [&](core::json::object_t &value) -> bool {
            return dispatch(handler, message, buffer, value, trace_info);
          },
          [&](core::json::array_t &value) -> bool {
            return dispatch(handler, message, buffer, value, trace_info);
          },
      },
      root);
}

bool ParserPrivate::dispatch(
    Handler &handler,
    const std::string_view &message,
    core::json::Buffer &,
    core::json::object_t &root,
    const server::TraceInfo &trace_info) {
  bool dispatched = false;
  for (auto [key, value] : root) {
    auto field = ResultField(key);
    switch (field) {
      case ResultField::UNDEFINED:
      case ResultField::UNKNOWN: break;
      case ResultField::EVENT: {
        auto event = Event(value);
        switch (event) {
          case Event::UNDEFINED: LOG(FATAL)("Unexpected"); break;
          case Event::UNKNOWN: DLOG(FATAL)(R"(Unknown key="{}")", key); break;
          case Event::ERROR: {
            auto error = core::json::Parser::create<Error>(message);
            handler(error, trace_info);
            dispatched = true;
            break;
          }
          case Event::SYSTEM_STATUS: {
            auto system_status =
                core::json::Parser::create<SystemStatus>(message);
            handler(system_status, trace_info);
            dispatched = true;
            break;
          }
          case Event::PONG: {
            auto pong = core::json::Parser::create<Pong>(message);
            handler(pong, trace_info);
            dispatched = true;
            break;
          }
          case Event::HEARTBEAT: {
            auto heartbeat = core::json::Parser::create<Heartbeat>(message);
            handler(heartbeat, trace_info);
            dispatched = true;
            break;
          }
          case Event::SUBSCRIPTION_STATUS: {
            auto subscription_status =
                core::json::Parser::create<SubscriptionStatus>(message);
            handler(subscription_status, trace_info);
            dispatched = true;
            break;
          }
          case Event::ADD_ORDER_STATUS: {
            auto add_order_status =
                core::json::Parser::create<AddOrderStatus>(message);
            handler(add_order_status, trace_info);
            dispatched = true;
            break;
          }
          case Event::CANCEL_ORDER_STATUS:
            auto cancel_order_status =
                core::json::Parser::create<CancelOrderStatus>(message);
            handler(cancel_order_status, trace_info);
            dispatched = true;
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
    [[maybe_unused]] ParserPrivate::Handler &handler,
    [[maybe_unused]] const std::string_view &message,
    [[maybe_unused]] core::json::Buffer &buffer,
    [[maybe_unused]] Channel channel) {
  bool dispatched = false;
  /*
  core::json::Parser parser(message);
  auto root = parser.root();
  size_t offset = 0;
  Book book_1, book_2;
  for (auto value : std::get<core::json::array_t>(root)) {
    if (++offset == 1)
      continue;
    if (offset > (1 + data_count))
      break;
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
        LOG_IF(FATAL, data_count != 1)("Unexpected");
        Trade trade(
            value,
            buffer);
        handler(trade, pair);
        dispatched = true;
        break;
      }
      case Channel::SPREAD: {
        LOG_IF(FATAL, data_count != 1)("Unexpected");
        Spread spread(value);
        handler(spread, pair);
        dispatched = true;
        break;
      }
      case Channel::BOOK: {
        LOG_IF(FATAL, data_count < 1 || data_count > 2)("Unexpected");
        switch (offset) {
          case 2:
            book_1 = Book(value, buffer);
            break;
          case 3:
            book_2 = Book(value, buffer);
            break;
          default:
            LOG(FATAL)("Unexpected");
        }
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
  if (dispatched == false && channel == Channel::BOOK) {
    if (data_count == 2) {
      if (book_2.a.empty() == false) {
        LOG_IF(FATAL, book_1.a.empty() == false)("Unexpected");
        book_1.a = book_2.a;
      } else if (book_2.b.empty() == false) {
        LOG_IF(FATAL, book_1.b.empty() == false)("Unexpected");
        book_1.b = book_2.b;
      } else {
        LOG(FATAL)("Unexpected");
      }
    }
    handler(book_1, pair);
    dispatched = true;
  }
*/
  return dispatched;
}
}  // namespace

bool ParserPrivate::dispatch(
    Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    core::json::array_t &root,
    const server::TraceInfo &) {
  Channel channel = Channel::UNDEFINED;
  size_t offset = 0;
  for (auto value : root) {
    switch (offset) {
      case 1: {
        auto name = std::get<std::string_view>(value);
        // for example "book-10" --> "book"
        auto pos = name.find_first_of('-');
        if (pos != name.npos) name.remove_suffix(name.size() - pos);
        channel = Channel(name);
        DLOG_IF(FATAL, channel == Channel::UNKNOWN)
        (R"(Unknown channel="{}")", name);
        break;
      }
      default: break;
    }
    ++offset;
  }
  LOG_IF(FATAL, offset != 2)(R"(message={})", message);
  return dispatch2(handler, message, buffer, channel);
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

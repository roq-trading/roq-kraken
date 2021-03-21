/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/json/parser_public.h"

#include "roq/logging.h"

#include "roq/kraken/json/channel.h"
#include "roq/kraken/json/event.h"
#include "roq/kraken/json/result_field.h"

using namespace roq::literals;

namespace roq {
namespace kraken {
namespace json {

bool ParserPublic::dispatch(
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

bool ParserPublic::dispatch(
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
      case ResultField::UNKNOWN:
        break;
      case ResultField::EVENT: {
        auto event = Event(value);
        switch (event) {
          case Event::UNDEFINED:
            log::fatal("Unexpected"_sv);
            break;
          case Event::UNKNOWN:
            log::fatal(R"(Unknown key="{}")"_fmt, key);
            break;
          case Event::ERROR: {
            auto error = core::json::Parser::create<Error>(message);
            handler(error, trace_info);
            dispatched = true;
            break;
          }
          case Event::SYSTEM_STATUS: {
            auto system_status = core::json::Parser::create<SystemStatus>(message);
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
            auto subscription_status = core::json::Parser::create<SubscriptionStatus>(message);
            handler(subscription_status, trace_info);
            dispatched = true;
            break;
          }
          case Event::ADD_ORDER_STATUS: {
            throw std::runtime_error("addOrderStatus not supported"_s);
          }
          case Event::CANCEL_ORDER_STATUS:
            throw std::runtime_error("cancelOrderStatus not supported"_s);
        }
        break;
      }
    }
  }
  return dispatched;
}

namespace {
static bool dispatch2(
    ParserPublic::Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    const server::TraceInfo &trace_info,
    [[maybe_unused]] int64_t channel_id,
    Channel channel,
    const std::string_view &pair,
    size_t data_count) {
  /*
  log::debug(
      R"(channel_id={} channel={} pair={}, len(data)={})"_fmt,
      channel_id,
      channel,
      pair,
      data_count);
  */
  bool dispatched = false;
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
        log::fatal("Unexpected"_sv);
        break;
      case Channel::TICKER: {
        throw std::runtime_error("ticker not supported"_s);
      }
      case Channel::OHLC: {
        throw std::runtime_error("ohlc not supported"_s);
      }
      case Channel::TRADE: {
        if (ROQ_UNLIKELY(data_count != 1))
          log::fatal("Unexpected"_sv);
        Trade trade(value, buffer);
        handler(trade, pair, trace_info);
        dispatched = true;
        break;
      }
      case Channel::SPREAD: {
        if (ROQ_UNLIKELY(data_count != 1))
          log::fatal("Unexpected"_sv);
        Spread spread(value);
        handler(spread, pair, trace_info);
        dispatched = true;
        break;
      }
      case Channel::BOOK: {
        if (ROQ_UNLIKELY(data_count < 1 || data_count > 2))
          log::fatal("Unexpected"_sv);
        switch (offset) {
          case 2:
            book_1 = Book(value, buffer);
            break;
          case 3:
            book_2 = Book(value, buffer);
            break;
          default:
            log::fatal("Unexpected"_sv);
        }
        break;
      }
      case Channel::OWN_TRADES: {
        throw std::runtime_error("ownTrades not supported"_s);
      }
      case Channel::OPEN_ORDERS: {
        throw std::runtime_error("openOrders not supported"_s);
      }
    }
  }
  if (!dispatched && channel == Channel::BOOK) {
    if (data_count == 2) {
      if (!book_2.a.empty()) {
        if (ROQ_UNLIKELY(!book_1.a.empty()))
          log::fatal("Unexpected"_sv);
        book_1.a = book_2.a;
      } else if (!book_2.b.empty()) {
        if (ROQ_UNLIKELY(!book_1.b.empty()))
          log::fatal("Unexpected"_sv);
        book_1.b = book_2.b;
      } else {
        log::fatal("Unexpected"_sv);
      }
    }
    handler(book_1, pair, trace_info);
    dispatched = true;
  }
  return dispatched;
}
}  // namespace

bool ParserPublic::dispatch(
    Handler &handler,
    const std::string_view &message,
    core::json::Buffer &buffer,
    core::json::array_t &root,
    const server::TraceInfo &trace_info) {
  int64_t channel_id = 0;
  Channel channel = Channel::UNDEFINED;
  std::string_view pair;
  size_t offset = 0;
  size_t data_count = 0;
  for (auto value : root) {
    if (offset == 0) {
      channel_id = std::get<int64_t>(value);
      ++offset;
    } else {
      if (core::json::is_pod(value)) {
        switch (offset) {
          case 1: {
            auto name = std::get<std::string_view>(value);
            // for example "book-10" --> "book"
            auto pos = name.find_first_of('-');
            if (pos != name.npos)
              name.remove_suffix(name.size() - pos);
            channel = Channel(name);
#if !defined(NDEBUG)
            if (ROQ_UNLIKELY(channel == Channel::UNKNOWN))
              log::fatal(R"(Unknown channel="{}")"_fmt, name);
#endif
            break;
          }
          case 2:
            pair = std::get<std::string_view>(value);
            break;
        }
        ++offset;
      } else {
        ++data_count;
      }
    }
  }
  if (ROQ_UNLIKELY(offset != 3))
    log::fatal(R"(message="{}")"_fmt, message);
  return dispatch2(handler, message, buffer, trace_info, channel_id, channel, pair, data_count);
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

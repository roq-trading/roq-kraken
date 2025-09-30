/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/kraken/json/parser_private.hpp"

#include "roq/logging.hpp"

#include "roq/utils/patterns.hpp"

#include "roq/kraken/json/channel.hpp"
#include "roq/kraken/json/event.hpp"
#include "roq/kraken/json/result_field.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace json {

// === HELPERS ===

namespace {
template <typename T>
void dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj);
}
}  // namespace

// === IMPLEMENTATION ===

bool ParserPrivate::dispatch(
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, TraceInfo const &trace_info, bool allow_unknown_event_types) {
  // different parsing depending on object or array representation
  core::json::Parser parser{message};
  auto root = parser.root();
  return std::visit(
      utils::overloaded{
          [](core::json::Null const &) -> bool { throw std::bad_cast{}; },
          [](bool) -> bool { throw std::bad_cast{}; },
          [](int64_t) -> bool { throw std::bad_cast{}; },
          [](double) -> bool { throw std::bad_cast{}; },
          [](std::string_view const &) -> bool { throw std::bad_cast{}; },
          [&](core::json::Object &value) -> bool { return dispatch(handler, message, buffer_stack, value, trace_info); },
          [&](core::json::Array &value) -> bool { return dispatch(handler, message, buffer_stack, value, trace_info); },
      },
      root);
}

bool ParserPrivate::dispatch(
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, core::json::Object &root, TraceInfo const &trace_info) {
  bool dispatched = false;
  for (auto [key, value] : root) {
    auto field = ResultField{key};
    switch (field) {
      using enum ResultField::type_t;
      case UNDEFINED_INTERNAL:
        break;
      case UNKNOWN_INTERNAL:
        break;
      case EVENT: {
        auto event = Event{value};
        switch (event) {
          using enum Event::type_t;
          case UNDEFINED_INTERNAL:
            log::fatal("Unexpected"sv);
            break;
          case UNKNOWN_INTERNAL:
            log::fatal(R"(Unknown key="{}")"sv, key);
            break;
          case ERROR:
            dispatch_helper<Error>(handler, message, buffer_stack, trace_info);
            dispatched = true;
            break;
          case SYSTEM_STATUS:
            dispatch_helper<SystemStatus>(handler, message, buffer_stack, trace_info);
            dispatched = true;
            break;
          case PONG:
            dispatch_helper<Pong>(handler, message, buffer_stack, trace_info);
            dispatched = true;
            break;
          case HEARTBEAT:
            dispatch_helper<Heartbeat>(handler, message, buffer_stack, trace_info);
            dispatched = true;
            break;
          case SUBSCRIPTION_STATUS:
            dispatch_helper<SubscriptionStatus>(handler, message, buffer_stack, trace_info);
            dispatched = true;
            break;
          case ADD_ORDER_STATUS:
            dispatch_helper<AddOrderStatus>(handler, message, buffer_stack, trace_info);
            dispatched = true;
            break;
          case CANCEL_ORDER_STATUS:
            dispatch_helper<CancelOrderStatus>(handler, message, buffer_stack, trace_info);
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
bool dispatch2(
    [[maybe_unused]] ParserPrivate::Handler &handler,
    [[maybe_unused]] std::string_view const &message,
    [[maybe_unused]] core::json::BufferStack &buffer_stack,
    [[maybe_unused]] Channel channel) {
  bool dispatched = false;
  /*
  core::json::Parser parser(message);
  auto root = parser.root();
  size_t offset = 0;
  Book book_1, book_2;
  for (auto value : std::get<core::json::Array>(root)) {
    if (++offset == 1)
      continue;
    if (offset > (1 + data_count))
      break;
    switch (channel) {
      using enum Channel::type_t;
      case UNDEFINED_INTERNAL:
      case UNKNOWN_INTERNAL:
        log::fatal("Unexpected"sv);
        break;
      case TICKER: {
        throw RuntimeError{"ticker not supported"sv};
        break;
      }
      case OHLC: {
        throw RuntimeError{"ohlc not supported"sv};
        break;
      }
      case TRADE: {
        LOG_IF(FATAL, data_count != 1)("Unexpected"sv);
        Trade trade(
            value,
            buffer_stack);
        handler(trade, pair);
        dispatched = true;
        break;
      }
      case SPREAD: {
        LOG_IF(FATAL, data_count != 1)("Unexpected"sv);
        Spread spread(value);
        handler(spread, pair);
        dispatched = true;
        break;
      }
      case BOOK: {
        LOG_IF(FATAL, data_count < 1 || data_count > 2)("Unexpected"sv);
        switch (offset) {
          case 2:
            book_1 = Book(value, buffer_stack);
            break;
          case 3:
            book_2 = Book(value, buffer_stack);
            break;
          default:
            log::fatal("Unexpected"sv);
        }
        break;
      }
      case OWN_TRADES: {
        throw RuntimeError{"ownTrades not supported"sv};
        break;
      }
      case OPEN_ORDERS: {
        throw RuntimeError{"openOrders not supported"sv};
        break;
      }
    }
  }
  if (!dispatched && channel == Channel::BOOK) {
    if (data_count == 2) {
      if (!std::empty(book_2.a)) {
        LOG_IF(FATAL, !std::empty(book_1.a))("Unexpected"sv);
        book_1.a = book_2.a;
      } else if (!std::empty(book_2.b)) {
        LOG_IF(FATAL, !std::empty(book_1.b))("Unexpected"sv);
        book_1.b = book_2.b;
      } else {
        log::fatal("Unexpected"sv);
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
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, core::json::Array &root, TraceInfo const &) {
  Channel channel = Channel::UNDEFINED_INTERNAL;
  size_t offset = 0;
  for (auto value : root) {
    switch (offset) {
      case 1: {
        auto name = std::get<std::string_view>(value);
        // for example "book-10" --> "book"
        auto pos = name.find_first_of('-');
        if (pos != name.npos) {
          name.remove_suffix(std::size(name) - pos);
        }
        channel = Channel{name};
#ifndef NDEBUG
        if (channel == Channel::UNKNOWN_INTERNAL) [[unlikely]] {
          log::fatal(R"(Unknown channel="{}")"sv, name);
        }
#endif
        break;
      }
      default:
        break;
    }
    ++offset;
  }
  if (offset < 3) [[unlikely]] {
    log::fatal(R"(Unexpected: message="{}")"sv, message);
  }
  return dispatch2(handler, message, buffer_stack, channel);
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/kraken/json/parser_public.hpp"

#include "roq/logging.hpp"

#include "roq/utils/patterns.hpp"

#include "roq/kraken/json/channel.hpp"
#include "roq/kraken/json/event.hpp"
#include "roq/kraken/json/result_field.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace json {

namespace {
template <typename T>
void dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj);
}
}  // namespace

// === IMPLEMENTATION ===

bool ParserPublic::dispatch(
    Handler &handler,
    std::string_view const &message,
    core::json::BufferStack &buffer_stack,
    TraceInfo const &trace_info,
    [[maybe_unused]] bool allow_unknown_event_types) {
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

bool ParserPublic::dispatch(
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
        auto event = Event(value);
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
          case ADD_ORDER_STATUS: {
            throw RuntimeError{"addOrderStatus not supported"sv};
          }
          case CANCEL_ORDER_STATUS:
            throw RuntimeError{"cancelOrderStatus not supported"sv};
        }
        break;
      }
    }
  }
  return dispatched;
}

namespace {
bool dispatch2(
    ParserPublic::Handler &handler,
    std::string_view const &message,
    core::json::BufferStack &buffer_stack,
    TraceInfo const &trace_info,
    [[maybe_unused]] int64_t channel_id,
    Channel channel,
    std::string_view const &pair,
    size_t data_count) {
  /*
  log::debug(
      R"(channel_id={} channel={} pair={}, len(data)={})"sv,
      channel_id,
      channel,
      pair,
      data_count);
  */
  bool dispatched = false;
  core::json::Parser parser{message};
  auto root = parser.root();
  size_t offset = 0;
  Book book_1, book_2;
  auto &obj = std::get<core::json::Array>(root);
  for (auto value : obj) {
    if (++offset == 1) {
      continue;
    }
    if (offset > (1 + data_count)) {
      break;
    }
    switch (channel) {
      using enum Channel::type_t;
      case UNDEFINED_INTERNAL:
      case UNKNOWN_INTERNAL:
        log::fatal("Unexpected"sv);
        break;
      case TICKER:
        throw RuntimeError{"ticker not supported"sv};
      case OHLC:
        throw RuntimeError{"ohlc not supported"sv};
      case TRADE: {
        if (data_count != 1) [[unlikely]] {
          log::fatal("Unexpected"sv);
        }
        Trade trade{value, buffer_stack};
        Trace event{trace_info, trade};
        handler(event, pair);
        dispatched = true;
        break;
      }
      case SPREAD: {
        if (data_count != 1) [[unlikely]] {
          log::fatal("Unexpected"sv);
        }
        Spread spread{value};
        Trace event{trace_info, spread};
        handler(event, pair);
        dispatched = true;
        break;
      }
      case BOOK: {
        if (data_count < 1 || data_count > 2) [[unlikely]] {
          log::fatal("Unexpected"sv);
        }
        switch (offset) {
          case 2:
            book_1 = Book{value, buffer_stack};
            break;
          case 3:
            book_2 = Book{value, buffer_stack};
            break;
          default:
            log::fatal("Unexpected"sv);
        }
        break;
      }
      case OWN_TRADES:
        throw RuntimeError{"ownTrades not supported"sv};
      case OPEN_ORDERS:
        throw RuntimeError{"openOrders not supported"sv};
    }
  }
  if (!dispatched && channel == Channel::BOOK) {
    if (data_count == 2) {
      if (!std::empty(book_2.a)) {
        if (!std::empty(book_1.a)) [[unlikely]] {
          log::fatal("Unexpected"sv);
        }
        book_1.a = book_2.a;
      } else if (!std::empty(book_2.b)) {
        if (!std::empty(book_1.b)) [[unlikely]] {
          log::fatal("Unexpected"sv);
        }
        book_1.b = book_2.b;
      } else {
        log::fatal("Unexpected"sv);
      }
    }
    Trace event{trace_info, std::as_const(book_1)};
    handler(event, pair);
    dispatched = true;
  }
  return dispatched;
}
}  // namespace

bool ParserPublic::dispatch(
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, core::json::Array &root, TraceInfo const &trace_info) {
  int64_t channel_id = 0;
  Channel channel = Channel::UNDEFINED_INTERNAL;
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
  if (offset != 3) [[unlikely]] {
    log::fatal(R"(message="{}")"sv, message);
  }
  return dispatch2(handler, message, buffer_stack, trace_info, channel_id, channel, pair, data_count);
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

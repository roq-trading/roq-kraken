/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/json/parser.hpp"

#include "roq/logging.hpp"

#include "roq/utils/hash/fnv.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace json {

// === CONSTANTS ===

namespace {
constexpr auto const KEY_METHOD = "method"sv;
constexpr auto const KEY_CHANNEL = "channel"sv;
// methods
constexpr auto const METHOD_ERROR = "error"sv;
constexpr auto const METHOD_PONG = "pong"sv;
constexpr auto const METHOD_SUBSCRIBE = "subscribe"sv;
constexpr auto const METHOD_ADD_ORDER = "add_order"sv;
constexpr auto const METHOD_AMEND_ORDER = "amend_order"sv;
constexpr auto const METHOD_CANCEL_ORDER = "cancel_order"sv;
constexpr auto const METHOD_CANCEL_ALL = "cancel_all"sv;
// channels
constexpr auto const CHANNEL_STATUS = "status"sv;
constexpr auto const CHANNEL_HEARTBEAT = "heartbeat"sv;
constexpr auto const CHANNEL_INSTRUMENT = "instrument"sv;
constexpr auto const CHANNEL_TICKER = "ticker"sv;
constexpr auto const CHANNEL_TRADE = "trade"sv;
constexpr auto const CHANNEL_BOOK = "book"sv;
constexpr auto const CHANNEL_BALANCES = "balances"sv;
constexpr auto const CHANNEL_EXECUTIONS = "executions"sv;
}  // namespace

// === HELPERS ===

namespace {
template <typename T>
auto dispatch_helper(auto &handler, auto &message, auto &buffer_stack, auto &trace_info) {
  T obj{message, buffer_stack};
  create_trace_and_dispatch(handler, trace_info, obj);
  return true;
}
}  // namespace

// === IMPLEMENTATION ===

bool Parser::dispatch(
    Handler &handler, std::string_view const &message, core::json::BufferStack &buffer_stack, TraceInfo const &trace_info, bool allow_unknown_event_types) {
  auto result = false;
  auto helper = [&](auto &key, auto &value) {
    auto key_2 = utils::hash::FNV::compute(key);
    switch (key_2) {
      case utils::hash::FNV::compute(KEY_METHOD): {
        auto value_2 = utils::hash::FNV::compute(std::get<std::string_view>(value));
        switch (value_2) {
          case utils::hash::FNV::compute(METHOD_ERROR): {
            result = dispatch_helper<Error>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(METHOD_PONG): {
            result = dispatch_helper<Pong>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(METHOD_SUBSCRIBE): {
            result = dispatch_helper<Subscribe>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(METHOD_ADD_ORDER): {
            result = dispatch_helper<AddOrder>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(METHOD_AMEND_ORDER): {
            result = dispatch_helper<AmendOrder>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(METHOD_CANCEL_ORDER): {
            result = dispatch_helper<CancelOrder>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(METHOD_CANCEL_ALL): {
            result = dispatch_helper<CancelAll>(handler, message, buffer_stack, trace_info);
            break;
          }
        }
        break;
      }
      case utils::hash::FNV::compute(KEY_CHANNEL): {
        auto value_2 = utils::hash::FNV::compute(std::get<std::string_view>(value));
        switch (value_2) {
          case utils::hash::FNV::compute(CHANNEL_STATUS): {
            result = dispatch_helper<Status>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(CHANNEL_HEARTBEAT): {
            result = dispatch_helper<Heartbeat>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(CHANNEL_INSTRUMENT): {
            result = dispatch_helper<Instrument>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(CHANNEL_TICKER): {
            result = dispatch_helper<Ticker>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(CHANNEL_TRADE): {
            result = dispatch_helper<Trade>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(CHANNEL_BOOK): {
            result = dispatch_helper<Book>(handler, message, buffer_stack, trace_info);
            break;
          }
          // ...
          case utils::hash::FNV::compute(CHANNEL_BALANCES): {
            result = dispatch_helper<Balances>(handler, message, buffer_stack, trace_info);
            break;
          }
          case utils::hash::FNV::compute(CHANNEL_EXECUTIONS): {
            result = dispatch_helper<Executions>(handler, message, buffer_stack, trace_info);
            break;
          }
        }
        break;
      }
    }
    return result;
  };
  core::json::Parser::dispatch<core::json::Object>(helper, message);
  if (result || allow_unknown_event_types) {
    return result;
  }
  log::fatal(R"(Unexpected: message="{}")"sv, message);
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

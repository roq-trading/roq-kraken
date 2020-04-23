/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/web_socket_state.h"

#include <cassert>


namespace roq {
namespace kraken {

std::string_view WebSocketState::as_text() const {
  switch (_type) {
    case type_t::UNDEFINED: break;
    case type_t::SYMBOLS: return "SYMBOLS";
    case type_t::TRADING_BALANCE: return "TRADING_BALANCE";
    case type_t::ORDERS: return "ORDERS";
    case type_t::DONE: return "DONE";
  }
  return "UNDEFINED";
}

std::string_view WebSocketState::as_raw_text() const {
  switch (_type) {
    case type_t::UNDEFINED: break;
    case type_t::SYMBOLS: return "symbols";
    case type_t::TRADING_BALANCE: return "trading_balance";
    case type_t::ORDERS: return "orders";
    case type_t::DONE: return "done";
  }
  return "UNDEFINED";
}

}  // namespace kraken
}  // namespace roq


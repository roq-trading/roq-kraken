/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/api.hpp"

#include "roq/exceptions.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === IMPLEMENTATION ===

API API::create(Settings const &) {
  return {
      .order_management{
          .get_web_sockets_token = "/0/private/GetWebSocketsToken"sv,
          .balance = "/0/private/Balance"sv,
          .open_positions = "/0/private/OpenPositions"sv,
          .open_orders = "/0/private/OpenOrders"sv,
          .trades = "/0/private/QueryTrades"sv,
          .trade_balance = "/0/private/TradeBalance"sv,
      },
  };
}

}  // namespace kraken
}  // namespace roq

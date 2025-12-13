/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/api.hpp"

#include "roq/exceptions.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === IMPLEMENTATION ===

API API::create(Settings const &) {
  return {
      .market_data{
          .assets = "/0/public/Assets"sv,
          .asset_pairs = "/0/public/AssetPairs"sv,
      },
      .order_management{
          .get_web_sockets_token = "/0/private/GetWebSocketsToken"sv,
          .open_positions = "/0/private/OpenPositions"sv,
      },
  };
}

}  // namespace kraken
}  // namespace roq

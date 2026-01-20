/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/kraken/settings.hpp"

namespace roq {
namespace kraken {

struct API final {
  struct {
    std::string_view get_web_sockets_token;
    std::string_view balance;
    std::string_view open_positions;
    std::string_view open_orders;
    std::string_view trades;
    std::string_view trade_balance;
  } order_management;

  // factory
  static API create(Settings const &);
};

}  // namespace kraken
}  // namespace roq

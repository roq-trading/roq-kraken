/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <cstdint>

namespace roq {
namespace kraken {

enum class OrderEntryState : uint8_t {
  UNDEFINED = 0,
  TOKEN,
  BALANCE,
  TRADE_BALANCE,
  OPEN_POSITIONS,
  OPEN_ORDERS,
  DONE,
};

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/utility.h"

#include "roq/core/json/parser.h"

#include "roq/core/charconv/datetime.h"

#include "roq/kraken/json/order_type.h"
#include "roq/kraken/json/side.h"
#include "roq/kraken/json/update_type.h"

namespace roq {
namespace kraken {
namespace json {

template <typename T>
inline void update(
    T& result,
    const core::json::value_t& value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(
    std::chrono::nanoseconds& result,
    const core::json::value_t& value) {
  auto text = core::json::get<std::string_view>(value);
  result = core::charconv::to_datetime(text);
}

template <>
inline void update(
    OrderType& result,
    const core::json::value_t& value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(
    Side& result,
    const core::json::value_t& value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

template <>
inline void update(
    UpdateType& result,
    const core::json::value_t& value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}

inline roq::OrderType map(json::OrderType order_type) {
  switch (order_type) {
    case json::OrderType::UNDEFINED: break;
    case json::OrderType::UNKNOWN:   break;
    case json::OrderType::L:         return roq::OrderType::LIMIT;
    case json::OrderType::LIMIT:     return roq::OrderType::LIMIT;
    case json::OrderType::M:         return roq::OrderType::MARKET;
    case json::OrderType::MARKET:    return roq::OrderType::MARKET;
  }
  return roq::OrderType::UNDEFINED;
}

inline roq::Side map(json::Side side) {
  switch (side) {
    case json::Side::UNDEFINED: break;
    case json::Side::UNKNOWN:   break;
    case json::Side::B:         return roq::Side::BUY;
    case json::Side::BUY:       return roq::Side::BUY;
    case json::Side::S:         return roq::Side::SELL;
    case json::Side::SELL:      return roq::Side::SELL;
  }
  return roq::Side::UNDEFINED;
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

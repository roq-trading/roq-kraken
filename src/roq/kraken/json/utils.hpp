/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/json/parser.hpp"

#include "roq/core/charconv/datetime.hpp"

#include "roq/kraken/json/order_type.hpp"
#include "roq/kraken/json/side.hpp"

namespace roq {
namespace kraken {
namespace json {

template <typename T>
inline void update(T &result, core::json::Value const &value) {
  result = core::json::get<T>(value);
}

template <>
inline void update(std::chrono::milliseconds &result, core::json::Value const &value) {
  return std::visit(
      overloaded{
          [&](core::json::Null const &) { result = std::chrono::milliseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = std::chrono::milliseconds{static_cast<uint64_t>(value * int64_t{1000})}; },
          [&](double value) { result = std::chrono::milliseconds{static_cast<uint64_t>(value * 1.0e3)}; },
          [&](std::string_view const &value) {
            result = core::charconv::datetime_from_string<std::remove_reference<decltype(result)>::type>(value);
          },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

template <>
inline void update(std::chrono::microseconds &result, core::json::Value const &value) {
  return std::visit(
      overloaded{
          [&](core::json::Null const &) { result = std::chrono::microseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = std::chrono::microseconds{static_cast<uint64_t>(value * int64_t{1000000})}; },
          [&](double value) { result = std::chrono::microseconds{static_cast<uint64_t>(value * 1.0e6)}; },
          [&](std::string_view const &value) {
            result = core::charconv::datetime_from_string<std::remove_reference<decltype(result)>::type>(value);
          },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

inline roq::OrderType map(json::OrderType order_type) {
  switch (order_type) {
    using enum json::OrderType::type_t;
    case UNDEFINED:
      break;
    case UNKNOWN:
      break;
    case L:
      return roq::OrderType::LIMIT;
    case LIMIT:
      return roq::OrderType::LIMIT;
    case M:
      return roq::OrderType::MARKET;
    case MARKET:
      return roq::OrderType::MARKET;
  }
  return roq::OrderType::UNDEFINED;
}

inline roq::Side map(json::Side side) {
  switch (side) {
    using enum json::Side::type_t;
    case UNDEFINED:
      break;
    case UNKNOWN:
      break;
    case B:
      return roq::Side::BUY;
    case BUY:
      return roq::Side::BUY;
    case S:
      return roq::Side::SELL;
    case SELL:
      return roq::Side::SELL;
  }
  return roq::Side::UNDEFINED;
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

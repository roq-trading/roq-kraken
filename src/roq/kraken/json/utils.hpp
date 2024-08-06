/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/utils/patterns.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/charconv/datetime.hpp"

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
      utils::overloaded{
          [&](core::json::Null const &) { result = std::chrono::milliseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = std::chrono::milliseconds{static_cast<uint64_t>(value * 1000)}; },
          [&](double value) { result = std::chrono::milliseconds{static_cast<uint64_t>(value * 1.0e3)}; },
          [&](std::string_view const &value) { result = core::charconv::datetime_from_string<std::remove_reference<decltype(result)>::type>(value); },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

template <>
inline void update(std::chrono::microseconds &result, core::json::Value const &value) {
  return std::visit(
      utils::overloaded{
          [&](core::json::Null const &) { result = std::chrono::microseconds{}; },
          [](bool) { throw std::bad_cast{}; },
          [&](int64_t value) { result = std::chrono::microseconds{static_cast<uint64_t>(value * 1000000)}; },
          [&](double value) { result = std::chrono::microseconds{static_cast<uint64_t>(value * 1.0e6)}; },
          [&](std::string_view const &value) { result = core::charconv::datetime_from_string<std::remove_reference<decltype(result)>::type>(value); },
          [](core::json::Object const &) { throw std::bad_cast{}; },
          [](core::json::Array const &) { throw std::bad_cast{}; },
      },
      value);
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

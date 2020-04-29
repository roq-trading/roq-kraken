/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/core/utility.h"

#include "roq/core/json/parser.h"

#include "roq/core/charconv/datetime.h"

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
  result = std::chrono::milliseconds{core::json::get<uint64_t>(value)};
}

/*
template <>
inline void update(
    ReportType& result,
    const core::json::value_t& value) {
  using result_type = std::remove_reference<decltype(result)>::type;
  result = result_type(core::json::get<std::string_view>(value));
}
*/

}  // namespace json
}  // namespace kraken
}  // namespace roq

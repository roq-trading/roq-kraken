/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/kraken/json/order_type.hpp"
#include "roq/kraken/json/side.hpp"

#include "roq/order_type.hpp"
#include "roq/side.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<OrderType> Map<kraken::json::OrderType>::helper() const;

template <>
template <>
std::optional<Side> Map<kraken::json::Side>::helper() const;

}  // namespace roq

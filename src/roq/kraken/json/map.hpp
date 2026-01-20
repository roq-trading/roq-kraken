/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/kraken/json/order_type.hpp"
#include "roq/kraken/json/side.hpp"
#include "roq/kraken/json/status2.hpp"
#include "roq/kraken/json/type.hpp"

#include "roq/order_type.hpp"
#include "roq/side.hpp"
#include "roq/trading_status.hpp"
#include "roq/update_type.hpp"

#include "roq/map.hpp"

namespace roq {

template <>
template <>
std::optional<OrderType> Map<kraken::json::OrderType>::helper() const;

template <>
template <>
std::optional<Side> Map<kraken::json::Side>::helper() const;

template <>
template <>
std::optional<TradingStatus> Map<kraken::json::Status2>::helper() const;

template <>
template <>
std::optional<UpdateType> Map<kraken::json::Type>::helper() const;

}  // namespace roq

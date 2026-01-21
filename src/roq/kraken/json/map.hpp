/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/kraken/json/order_type.hpp"
#include "roq/kraken/json/pairs_status.hpp"
#include "roq/kraken/json/side.hpp"
#include "roq/kraken/json/time_in_force.hpp"
#include "roq/kraken/json/type.hpp"

#include "roq/order_type.hpp"
#include "roq/side.hpp"
#include "roq/time_in_force.hpp"
#include "roq/trading_status.hpp"
#include "roq/update_type.hpp"

#include "roq/map.hpp"

namespace roq {

// kraken => roq

template <>
template <>
std::optional<roq::OrderType> Map<kraken::json::OrderType>::helper() const;

template <>
template <>
std::optional<roq::Side> Map<kraken::json::Side>::helper() const;

template <>
template <>
std::optional<roq::TimeInForce> Map<kraken::json::TimeInForce>::helper() const;

template <>
template <>
std::optional<roq::TradingStatus> Map<kraken::json::PairsStatus>::helper() const;

template <>
template <>
std::optional<roq::UpdateType> Map<kraken::json::Type>::helper() const;

// roq => kraken

template <>
template <>
std::optional<kraken::json::OrderType> Map<roq::OrderType>::helper() const;

template <>
template <>
std::optional<kraken::json::Side> Map<roq::Side>::helper() const;

template <>
template <>
std::optional<kraken::json::TimeInForce> Map<roq::TimeInForce>::helper() const;

}  // namespace roq

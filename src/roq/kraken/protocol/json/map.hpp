/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/kraken/protocol/json/liquidity_ind.hpp"
#include "roq/kraken/protocol/json/order_status.hpp"
#include "roq/kraken/protocol/json/order_type.hpp"
#include "roq/kraken/protocol/json/pairs_status.hpp"
#include "roq/kraken/protocol/json/side.hpp"
#include "roq/kraken/protocol/json/time_in_force.hpp"
#include "roq/kraken/protocol/json/time_in_force2.hpp"
#include "roq/kraken/protocol/json/type.hpp"

#include "roq/liquidity.hpp"
#include "roq/order_status.hpp"
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
std::optional<roq::Liquidity> Map<kraken::protocol::json::LiquidityInd>::helper() const;

template <>
template <>
std::optional<roq::OrderStatus> Map<kraken::protocol::json::OrderStatus>::helper() const;

template <>
template <>
std::optional<roq::OrderType> Map<kraken::protocol::json::OrderType>::helper() const;

template <>
template <>
std::optional<roq::Side> Map<kraken::protocol::json::Side>::helper() const;

template <>
template <>
std::optional<roq::TimeInForce> Map<kraken::protocol::json::TimeInForce>::helper() const;

template <>
template <>
std::optional<roq::TimeInForce> Map<kraken::protocol::json::TimeInForce2>::helper() const;

template <>
template <>
std::optional<roq::TradingStatus> Map<kraken::protocol::json::PairsStatus>::helper() const;

template <>
template <>
std::optional<roq::UpdateType> Map<kraken::protocol::json::Type>::helper() const;

// roq => kraken

template <>
template <>
std::optional<kraken::protocol::json::OrderType> Map<roq::OrderType>::helper() const;

template <>
template <>
std::optional<kraken::protocol::json::Side> Map<roq::Side>::helper() const;

template <>
template <>
std::optional<kraken::protocol::json::TimeInForce> Map<roq::TimeInForce>::helper() const;

}  // namespace roq

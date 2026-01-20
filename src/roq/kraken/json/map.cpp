/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/json/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// kraken::json => roq

// kraken::json::OrderType => roq::OrderType

template <>
template <>
constexpr Helper<kraken::json::OrderType>::operator std::optional<roq::OrderType>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::json::OrderType::type_t;
    case UNDEFINED_INTERNAL:
      return roq::OrderType::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::OrderType::UNDEFINED;
    case L:
      return roq::OrderType::LIMIT;
    case LIMIT:
      return roq::OrderType::LIMIT;
    case M:
      return roq::OrderType::MARKET;
    case MARKET:
      return roq::OrderType::MARKET;
  }
  return {};
}

static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::UNDEFINED_INTERNAL}} == roq::OrderType::UNDEFINED);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::L}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::M}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::MARKET}} == roq::OrderType::MARKET);

template <>
template <>
std::optional<roq::OrderType> Map<kraken::json::OrderType>::helper() const {
  return Helper{args_};
}

// kraken::json::Side => roq::Side

template <>
template <>
constexpr Helper<kraken::json::Side>::operator std::optional<roq::Side>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::json::Side::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Side::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Side::UNDEFINED;
    case B:
      return roq::Side::BUY;
    case BUY:
      return roq::Side::BUY;
    case S:
      return roq::Side::SELL;
    case SELL:
      return roq::Side::SELL;
  }
  return {};
}

static_assert(Helper{kraken::json::Side{kraken::json::Side::UNDEFINED_INTERNAL}} == roq::Side::UNDEFINED);
static_assert(Helper{kraken::json::Side{kraken::json::Side::B}} == roq::Side::BUY);
static_assert(Helper{kraken::json::Side{kraken::json::Side::BUY}} == roq::Side::BUY);
static_assert(Helper{kraken::json::Side{kraken::json::Side::S}} == roq::Side::SELL);
static_assert(Helper{kraken::json::Side{kraken::json::Side::SELL}} == roq::Side::SELL);

template <>
template <>
std::optional<roq::Side> Map<kraken::json::Side>::helper() const {
  return Helper{args_};
}

// kraken::json::PairsStatus => roq::TradingStatus

template <>
template <>
constexpr Helper<kraken::json::PairsStatus>::operator std::optional<roq::TradingStatus>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::json::PairsStatus::type_t;
    case UNDEFINED_INTERNAL:
      return roq::TradingStatus::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::TradingStatus::UNDEFINED;
    case CANCEL_ONLY:
      return roq::TradingStatus::CLOSE;
    case DELISTED:
      return roq::TradingStatus::CLOSE;  // ???
    case LIMIT_ONLY:
      return roq::TradingStatus::CLOSE;
    case MAINTENANCE:
      return roq::TradingStatus::CLOSE;
    case ONLINE:
      return roq::TradingStatus::OPEN;
    case POST_ONLY:
      return roq::TradingStatus::CLOSE;
    case REDUCE_ONLY:
      return roq::TradingStatus::CLOSE;
    case WORK_IN_PROGRESS:
      return roq::TradingStatus::CLOSE;
  }
  return {};
}

static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::UNDEFINED_INTERNAL}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::CANCEL_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::DELISTED}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::LIMIT_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::MAINTENANCE}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::ONLINE}} == roq::TradingStatus::OPEN);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::POST_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::REDUCE_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::json::PairsStatus{kraken::json::PairsStatus::WORK_IN_PROGRESS}} == roq::TradingStatus::CLOSE);

template <>
template <>
std::optional<roq::TradingStatus> Map<kraken::json::PairsStatus>::helper() const {
  return Helper{args_};
}

// kraken::json::Type => roq::UpdateType

template <>
template <>
constexpr Helper<kraken::json::Type>::operator std::optional<roq::UpdateType>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::json::Type::type_t;
    case UNDEFINED_INTERNAL:
      return roq::UpdateType::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::UpdateType::UNDEFINED;
    case SNAPSHOT:
      return roq::UpdateType::SNAPSHOT;
    case UPDATE:
      return roq::UpdateType::INCREMENTAL;
  }
  return {};
}

static_assert(Helper{kraken::json::Type{kraken::json::Type::UNDEFINED_INTERNAL}} == roq::UpdateType::UNDEFINED);
static_assert(Helper{kraken::json::Type{kraken::json::Type::SNAPSHOT}} == roq::UpdateType::SNAPSHOT);
static_assert(Helper{kraken::json::Type{kraken::json::Type::UPDATE}} == roq::UpdateType::INCREMENTAL);

template <>
template <>
std::optional<roq::UpdateType> Map<kraken::json::Type>::helper() const {
  return Helper{args_};
}

}  // namespace roq

/* Copyright (c) 2017-2025, Hans Erik Thrane */

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
    case UNDEFINED__:
      return roq::OrderType::UNDEFINED;
    case UNKNOWN__:
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

static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::UNDEFINED__}} == roq::OrderType::UNDEFINED);
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
    case UNDEFINED__:
      return roq::Side::UNDEFINED;
    case UNKNOWN__:
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

static_assert(Helper{kraken::json::Side{kraken::json::Side::UNDEFINED__}} == roq::Side::UNDEFINED);
static_assert(Helper{kraken::json::Side{kraken::json::Side::B}} == roq::Side::BUY);
static_assert(Helper{kraken::json::Side{kraken::json::Side::BUY}} == roq::Side::BUY);
static_assert(Helper{kraken::json::Side{kraken::json::Side::S}} == roq::Side::SELL);
static_assert(Helper{kraken::json::Side{kraken::json::Side::SELL}} == roq::Side::SELL);

template <>
template <>
std::optional<roq::Side> Map<kraken::json::Side>::helper() const {
  return Helper{args_};
}

}  // namespace roq

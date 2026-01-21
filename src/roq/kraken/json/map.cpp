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
    case LIMIT:
      return roq::OrderType::LIMIT;
    case MARKET:
      return roq::OrderType::MARKET;
    case ICEBERG:
      return roq::OrderType::UNDEFINED;
    case STOP_LOSS:
      return roq::OrderType::MARKET;
    case STOP_LOSS_LIMIT:
      return roq::OrderType::LIMIT;
    case TAKE_PROFIT:
      return roq::OrderType::MARKET;
    case TAKE_PROFIT_LIMIT:
      return roq::OrderType::LIMIT;
    case TRAILING_STOP:
      return roq::OrderType::MARKET;
    case TRAILING_STOP_LIMIT:
      return roq::OrderType::LIMIT;
    case SETTLE_POSITION:
      return roq::OrderType::MARKET;
  }
  return {};
}

static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::UNDEFINED_INTERNAL}} == roq::OrderType::UNDEFINED);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::MARKET}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::ICEBERG}} == roq::OrderType::UNDEFINED);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::STOP_LOSS}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::STOP_LOSS_LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::TAKE_PROFIT}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::TAKE_PROFIT_LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::TRAILING_STOP}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::TRAILING_STOP_LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::json::OrderType{kraken::json::OrderType::SETTLE_POSITION}} == roq::OrderType::MARKET);

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
    case BUY:
      return roq::Side::BUY;
    case SELL:
      return roq::Side::SELL;
  }
  return {};
}

static_assert(Helper{kraken::json::Side{kraken::json::Side::UNDEFINED_INTERNAL}} == roq::Side::UNDEFINED);
static_assert(Helper{kraken::json::Side{kraken::json::Side::BUY}} == roq::Side::BUY);
static_assert(Helper{kraken::json::Side{kraken::json::Side::SELL}} == roq::Side::SELL);

template <>
template <>
std::optional<roq::Side> Map<kraken::json::Side>::helper() const {
  return Helper{args_};
}

// kraken::json::TimeInForce => roq::TimeInForce

template <>
template <>
constexpr Helper<kraken::json::TimeInForce>::operator std::optional<roq::TimeInForce>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::json::TimeInForce::type_t;
    case UNDEFINED_INTERNAL:
      return roq::TimeInForce::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::TimeInForce::UNDEFINED;
    case GTC:
      return roq::TimeInForce::GTC;
    case GTD:
      return roq::TimeInForce::GTD;
    case IOC:
      return roq::TimeInForce::IOC;
  }
  return {};
}

static_assert(Helper{kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL}} == roq::TimeInForce::UNDEFINED);
static_assert(Helper{kraken::json::TimeInForce{kraken::json::TimeInForce::GTC}} == roq::TimeInForce::GTC);
static_assert(Helper{kraken::json::TimeInForce{kraken::json::TimeInForce::GTD}} == roq::TimeInForce::GTD);
static_assert(Helper{kraken::json::TimeInForce{kraken::json::TimeInForce::IOC}} == roq::TimeInForce::IOC);

template <>
template <>
std::optional<roq::TimeInForce> Map<kraken::json::TimeInForce>::helper() const {
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

// roq => kraken::json

// roq::OrderType => kraken::json::OrderType

template <>
template <>
constexpr Helper<roq::OrderType>::operator std::optional<kraken::json::OrderType>() const {
  switch (std::get<0>(args_)) {
    using enum roq::OrderType;
    case UNDEFINED:
      return kraken::json::OrderType::UNDEFINED_INTERNAL;
    case MARKET:
      return kraken::json::OrderType::MARKET;
    case LIMIT:
      return kraken::json::OrderType::LIMIT;
  }
  return {};
}

static_assert(Helper{roq::OrderType::UNDEFINED} == kraken::json::OrderType{kraken::json::OrderType::UNDEFINED_INTERNAL});
static_assert(Helper{roq::OrderType::MARKET} == kraken::json::OrderType{kraken::json::OrderType::MARKET});
static_assert(Helper{roq::OrderType::LIMIT} == kraken::json::OrderType{kraken::json::OrderType::LIMIT});

template <>
template <>
std::optional<kraken::json::OrderType> Map<roq::OrderType>::helper() const {
  return Helper{args_};
}

// roq::Side => kraken::json::Side

template <>
template <>
constexpr Helper<roq::Side>::operator std::optional<kraken::json::Side>() const {
  switch (std::get<0>(args_)) {
    using enum roq::Side;
    case UNDEFINED:
      return kraken::json::Side::UNDEFINED_INTERNAL;
    case BUY:
      return kraken::json::Side::BUY;
    case SELL:
      return kraken::json::Side::SELL;
  }
  return {};
}

static_assert(Helper{roq::Side::UNDEFINED} == kraken::json::Side{kraken::json::Side::UNDEFINED_INTERNAL});
static_assert(Helper{roq::Side::BUY} == kraken::json::Side{kraken::json::Side::BUY});
static_assert(Helper{roq::Side::SELL} == kraken::json::Side{kraken::json::Side::SELL});

template <>
template <>
std::optional<kraken::json::Side> Map<roq::Side>::helper() const {
  return Helper{args_};
}

// roq::TimeInForce => kraken::json::TimeInForce

template <>
template <>
constexpr Helper<roq::TimeInForce>::operator std::optional<kraken::json::TimeInForce>() const {
  switch (std::get<0>(args_)) {
    using enum roq::TimeInForce;
    case UNDEFINED:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFD:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTC:
      return kraken::json::TimeInForce::GTC;
    case OPG:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case IOC:
      return kraken::json::TimeInForce::IOC;
    case FOK:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTX:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTD:
      return kraken::json::TimeInForce::GTD;
    case AT_THE_CLOSE:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GOOD_THROUGH_CROSSING:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case AT_CROSSING:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GOOD_FOR_TIME:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFA:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFM:
      return kraken::json::TimeInForce::UNDEFINED_INTERNAL;
  }
  return {};
}

static_assert(Helper{roq::TimeInForce::UNDEFINED} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFD} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTC} == kraken::json::TimeInForce{kraken::json::TimeInForce::GTC});
static_assert(Helper{roq::TimeInForce::OPG} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::IOC} == kraken::json::TimeInForce{kraken::json::TimeInForce::IOC});
static_assert(Helper{roq::TimeInForce::FOK} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTX} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTD} == kraken::json::TimeInForce{kraken::json::TimeInForce::GTD});
static_assert(Helper{roq::TimeInForce::AT_THE_CLOSE} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GOOD_THROUGH_CROSSING} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::AT_CROSSING} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GOOD_FOR_TIME} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFA} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFM} == kraken::json::TimeInForce{kraken::json::TimeInForce::UNDEFINED_INTERNAL});

template <>
template <>
std::optional<kraken::json::TimeInForce> Map<roq::TimeInForce>::helper() const {
  return Helper{args_};
}

}  // namespace roq

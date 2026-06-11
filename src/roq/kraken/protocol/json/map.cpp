/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {

namespace {
template <typename... Args>
using Helper = detail::MapHelper<Args...>;
}

// kraken::json => roq

// kraken::protocol::json::LiquidityInd => roq::Liquidity

template <>
template <>
constexpr Helper<kraken::protocol::json::LiquidityInd>::operator std::optional<roq::Liquidity>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::LiquidityInd::type_t;
    case UNDEFINED_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::Liquidity::UNDEFINED;
    case MAKER:
      return roq::Liquidity::MAKER;
    case TAKER:
      return roq::Liquidity::TAKER;
  }
  return {};
}

static_assert(Helper{kraken::protocol::json::LiquidityInd{kraken::protocol::json::LiquidityInd::UNDEFINED_INTERNAL}} == roq::Liquidity::UNDEFINED);
static_assert(Helper{kraken::protocol::json::LiquidityInd{kraken::protocol::json::LiquidityInd::MAKER}} == roq::Liquidity::MAKER);
static_assert(Helper{kraken::protocol::json::LiquidityInd{kraken::protocol::json::LiquidityInd::TAKER}} == roq::Liquidity::TAKER);

template <>
template <>
std::optional<roq::Liquidity> Map<kraken::protocol::json::LiquidityInd>::helper() const {
  return Helper{args_};
}

// kraken::protocol::json::OrderStatus => roq::OrderStatus

template <>
template <>
constexpr Helper<kraken::protocol::json::OrderStatus>::operator std::optional<roq::OrderStatus>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::OrderStatus::type_t;
    case UNDEFINED_INTERNAL:
      return roq::OrderStatus::UNDEFINED;
    case UNKNOWN_INTERNAL:
      return roq::OrderStatus::UNDEFINED;
    case PENDING_NEW:
      return roq::OrderStatus::ACCEPTED;
    case NEW:
      return roq::OrderStatus::WORKING;
    case PARTIALLY_FILLED:
      return roq::OrderStatus::WORKING;
    case FILLED:
      return roq::OrderStatus::COMPLETED;
    case CANCELED:
      return roq::OrderStatus::CANCELED;
    case EXPIRED:
      return roq::OrderStatus::EXPIRED;
  }
  return {};
}

static_assert(Helper{kraken::protocol::json::OrderStatus{kraken::protocol::json::OrderStatus::UNDEFINED_INTERNAL}} == roq::OrderStatus::UNDEFINED);
static_assert(Helper{kraken::protocol::json::OrderStatus{kraken::protocol::json::OrderStatus::PENDING_NEW}} == roq::OrderStatus::ACCEPTED);
static_assert(Helper{kraken::protocol::json::OrderStatus{kraken::protocol::json::OrderStatus::NEW}} == roq::OrderStatus::WORKING);
static_assert(Helper{kraken::protocol::json::OrderStatus{kraken::protocol::json::OrderStatus::PARTIALLY_FILLED}} == roq::OrderStatus::WORKING);
static_assert(Helper{kraken::protocol::json::OrderStatus{kraken::protocol::json::OrderStatus::FILLED}} == roq::OrderStatus::COMPLETED);
static_assert(Helper{kraken::protocol::json::OrderStatus{kraken::protocol::json::OrderStatus::CANCELED}} == roq::OrderStatus::CANCELED);
static_assert(Helper{kraken::protocol::json::OrderStatus{kraken::protocol::json::OrderStatus::EXPIRED}} == roq::OrderStatus::EXPIRED);

template <>
template <>
std::optional<roq::OrderStatus> Map<kraken::protocol::json::OrderStatus>::helper() const {
  return Helper{args_};
}

// kraken::protocol::json::OrderType => roq::OrderType

template <>
template <>
constexpr Helper<kraken::protocol::json::OrderType>::operator std::optional<roq::OrderType>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::OrderType::type_t;
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

static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::UNDEFINED_INTERNAL}} == roq::OrderType::UNDEFINED);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::MARKET}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::ICEBERG}} == roq::OrderType::UNDEFINED);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::STOP_LOSS}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::STOP_LOSS_LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::TAKE_PROFIT}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::TAKE_PROFIT_LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::TRAILING_STOP}} == roq::OrderType::MARKET);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::TRAILING_STOP_LIMIT}} == roq::OrderType::LIMIT);
static_assert(Helper{kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::SETTLE_POSITION}} == roq::OrderType::MARKET);

template <>
template <>
std::optional<roq::OrderType> Map<kraken::protocol::json::OrderType>::helper() const {
  return Helper{args_};
}

// kraken::protocol::json::Side => roq::Side

template <>
template <>
constexpr Helper<kraken::protocol::json::Side>::operator std::optional<roq::Side>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::Side::type_t;
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

static_assert(Helper{kraken::protocol::json::Side{kraken::protocol::json::Side::UNDEFINED_INTERNAL}} == roq::Side::UNDEFINED);
static_assert(Helper{kraken::protocol::json::Side{kraken::protocol::json::Side::BUY}} == roq::Side::BUY);
static_assert(Helper{kraken::protocol::json::Side{kraken::protocol::json::Side::SELL}} == roq::Side::SELL);

template <>
template <>
std::optional<roq::Side> Map<kraken::protocol::json::Side>::helper() const {
  return Helper{args_};
}

// kraken::protocol::json::TimeInForce => roq::TimeInForce

template <>
template <>
constexpr Helper<kraken::protocol::json::TimeInForce>::operator std::optional<roq::TimeInForce>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::TimeInForce::type_t;
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

static_assert(Helper{kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL}} == roq::TimeInForce::UNDEFINED);
static_assert(Helper{kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::GTC}} == roq::TimeInForce::GTC);
static_assert(Helper{kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::GTD}} == roq::TimeInForce::GTD);
static_assert(Helper{kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::IOC}} == roq::TimeInForce::IOC);

template <>
template <>
std::optional<roq::TimeInForce> Map<kraken::protocol::json::TimeInForce>::helper() const {
  return Helper{args_};
}

// kraken::protocol::json::TimeInForce2 => roq::TimeInForce

template <>
template <>
constexpr Helper<kraken::protocol::json::TimeInForce2>::operator std::optional<roq::TimeInForce>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::TimeInForce2::type_t;
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

static_assert(Helper{kraken::protocol::json::TimeInForce2{kraken::protocol::json::TimeInForce2::UNDEFINED_INTERNAL}} == roq::TimeInForce::UNDEFINED);
static_assert(Helper{kraken::protocol::json::TimeInForce2{kraken::protocol::json::TimeInForce2::GTC}} == roq::TimeInForce::GTC);
static_assert(Helper{kraken::protocol::json::TimeInForce2{kraken::protocol::json::TimeInForce2::GTD}} == roq::TimeInForce::GTD);
static_assert(Helper{kraken::protocol::json::TimeInForce2{kraken::protocol::json::TimeInForce2::IOC}} == roq::TimeInForce::IOC);

template <>
template <>
std::optional<roq::TimeInForce> Map<kraken::protocol::json::TimeInForce2>::helper() const {
  return Helper{args_};
}

// kraken::protocol::json::PairsStatus => roq::TradingStatus

template <>
template <>
constexpr Helper<kraken::protocol::json::PairsStatus>::operator std::optional<roq::TradingStatus>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::PairsStatus::type_t;
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

static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::UNDEFINED_INTERNAL}} == roq::TradingStatus::UNDEFINED);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::CANCEL_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::DELISTED}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::LIMIT_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::MAINTENANCE}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::ONLINE}} == roq::TradingStatus::OPEN);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::POST_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::REDUCE_ONLY}} == roq::TradingStatus::CLOSE);
static_assert(Helper{kraken::protocol::json::PairsStatus{kraken::protocol::json::PairsStatus::WORK_IN_PROGRESS}} == roq::TradingStatus::CLOSE);

template <>
template <>
std::optional<roq::TradingStatus> Map<kraken::protocol::json::PairsStatus>::helper() const {
  return Helper{args_};
}

// kraken::protocol::json::Type => roq::UpdateType

template <>
template <>
constexpr Helper<kraken::protocol::json::Type>::operator std::optional<roq::UpdateType>() const {
  switch (std::get<0>(args_)) {
    using enum kraken::protocol::json::Type::type_t;
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

static_assert(Helper{kraken::protocol::json::Type{kraken::protocol::json::Type::UNDEFINED_INTERNAL}} == roq::UpdateType::UNDEFINED);
static_assert(Helper{kraken::protocol::json::Type{kraken::protocol::json::Type::SNAPSHOT}} == roq::UpdateType::SNAPSHOT);
static_assert(Helper{kraken::protocol::json::Type{kraken::protocol::json::Type::UPDATE}} == roq::UpdateType::INCREMENTAL);

template <>
template <>
std::optional<roq::UpdateType> Map<kraken::protocol::json::Type>::helper() const {
  return Helper{args_};
}

// roq => kraken::json

// roq::OrderType => kraken::protocol::json::OrderType

template <>
template <>
constexpr Helper<roq::OrderType>::operator std::optional<kraken::protocol::json::OrderType>() const {
  switch (std::get<0>(args_)) {
    using enum roq::OrderType;
    case UNDEFINED:
      return kraken::protocol::json::OrderType::UNDEFINED_INTERNAL;
    case MARKET:
      return kraken::protocol::json::OrderType::MARKET;
    case LIMIT:
      return kraken::protocol::json::OrderType::LIMIT;
  }
  return {};
}

static_assert(Helper{roq::OrderType::UNDEFINED} == kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::UNDEFINED_INTERNAL});
static_assert(Helper{roq::OrderType::MARKET} == kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::MARKET});
static_assert(Helper{roq::OrderType::LIMIT} == kraken::protocol::json::OrderType{kraken::protocol::json::OrderType::LIMIT});

template <>
template <>
std::optional<kraken::protocol::json::OrderType> Map<roq::OrderType>::helper() const {
  return Helper{args_};
}

// roq::Side => kraken::protocol::json::Side

template <>
template <>
constexpr Helper<roq::Side>::operator std::optional<kraken::protocol::json::Side>() const {
  switch (std::get<0>(args_)) {
    using enum roq::Side;
    case UNDEFINED:
      return kraken::protocol::json::Side::UNDEFINED_INTERNAL;
    case BUY:
      return kraken::protocol::json::Side::BUY;
    case SELL:
      return kraken::protocol::json::Side::SELL;
  }
  return {};
}

static_assert(Helper{roq::Side::UNDEFINED} == kraken::protocol::json::Side{kraken::protocol::json::Side::UNDEFINED_INTERNAL});
static_assert(Helper{roq::Side::BUY} == kraken::protocol::json::Side{kraken::protocol::json::Side::BUY});
static_assert(Helper{roq::Side::SELL} == kraken::protocol::json::Side{kraken::protocol::json::Side::SELL});

template <>
template <>
std::optional<kraken::protocol::json::Side> Map<roq::Side>::helper() const {
  return Helper{args_};
}

// roq::TimeInForce => kraken::protocol::json::TimeInForce

template <>
template <>
constexpr Helper<roq::TimeInForce>::operator std::optional<kraken::protocol::json::TimeInForce>() const {
  switch (std::get<0>(args_)) {
    using enum roq::TimeInForce;
    case UNDEFINED:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFD:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTC:
      return kraken::protocol::json::TimeInForce::GTC;
    case OPG:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case IOC:
      return kraken::protocol::json::TimeInForce::IOC;
    case FOK:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTX:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GTD:
      return kraken::protocol::json::TimeInForce::GTD;
    case AT_THE_CLOSE:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GOOD_THROUGH_CROSSING:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case AT_CROSSING:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GOOD_FOR_TIME:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFA:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
    case GFM:
      return kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL;
  }
  return {};
}

static_assert(Helper{roq::TimeInForce::UNDEFINED} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFD} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTC} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::GTC});
static_assert(Helper{roq::TimeInForce::OPG} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::IOC} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::IOC});
static_assert(Helper{roq::TimeInForce::FOK} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTX} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GTD} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::GTD});
static_assert(Helper{roq::TimeInForce::AT_THE_CLOSE} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GOOD_THROUGH_CROSSING} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::AT_CROSSING} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GOOD_FOR_TIME} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFA} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});
static_assert(Helper{roq::TimeInForce::GFM} == kraken::protocol::json::TimeInForce{kraken::protocol::json::TimeInForce::UNDEFINED_INTERNAL});

template <>
template <>
std::optional<kraken::protocol::json::TimeInForce> Map<roq::TimeInForce>::helper() const {
  return Helper{args_};
}

}  // namespace roq

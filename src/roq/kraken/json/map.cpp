/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/kraken/json/map.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace json {

// === HELPERS ===

namespace {
// note! constexpr helper for static testing
template <typename... Args>
struct Helper final {
  explicit constexpr Helper(std::tuple<Args...> const &args) : args_{args} {}
  explicit constexpr Helper(Args &&...args_) : args_{std::forward<Args>(args_)...} {}

  template <typename R>
  constexpr operator R();

 private:
  std::tuple<Args...> const args_;
};

// ==> roq

// OrderType ==> roq::OrderType

template <>
template <>
constexpr Helper<OrderType>::operator roq::OrderType() {
  switch (std::get<0>(args_)) {
    using enum json::OrderType::type_t;
    case UNDEFINED__:
      return {};
    case UNKNOWN__:
      break;
    case L:
      return roq::OrderType::LIMIT;
    case LIMIT:
      return roq::OrderType::LIMIT;
    case M:
      return roq::OrderType::MARKET;
    case MARKET:
      return roq::OrderType::MARKET;
  }
  roq::log::fatal("Unexpected"sv);
}

static_assert(static_cast<roq::OrderType>(Helper{OrderType{OrderType::UNDEFINED__}}) == roq::OrderType::UNDEFINED);
static_assert(static_cast<roq::OrderType>(Helper{OrderType{OrderType::L}}) == roq::OrderType::LIMIT);
static_assert(static_cast<roq::OrderType>(Helper{OrderType{OrderType::LIMIT}}) == roq::OrderType::LIMIT);
static_assert(static_cast<roq::OrderType>(Helper{OrderType{OrderType::M}}) == roq::OrderType::MARKET);
static_assert(static_cast<roq::OrderType>(Helper{OrderType{OrderType::MARKET}}) == roq::OrderType::MARKET);

// Side ==> roq::Side

template <>
template <>
constexpr Helper<Side>::operator roq::Side() {
  switch (std::get<0>(args_)) {
    using enum Side::type_t;
    case UNDEFINED__:
      return {};
    case UNKNOWN__:
      break;
    case B:
      return roq::Side::BUY;
    case BUY:
      return roq::Side::BUY;
    case S:
      return roq::Side::SELL;
    case SELL:
      return roq::Side::SELL;
  }
  roq::log::fatal("Unexpected"sv);
}

static_assert(static_cast<roq::Side>(Helper{Side{Side::UNDEFINED__}}) == roq::Side::UNDEFINED);
static_assert(static_cast<roq::Side>(Helper{Side{Side::B}}) == roq::Side::BUY);
static_assert(static_cast<roq::Side>(Helper{Side{Side::BUY}}) == roq::Side::BUY);
static_assert(static_cast<roq::Side>(Helper{Side{Side::S}}) == roq::Side::SELL);
static_assert(static_cast<roq::Side>(Helper{Side{Side::SELL}}) == roq::Side::SELL);

// roq ==>
}  // namespace

// === IMPLEMENTATION ===

// ==> roq

template <>
template <>
Map<OrderType>::operator roq::OrderType() {
  return Helper{args_};
}

template <>
template <>
Map<Side>::operator roq::Side() {
  return Helper{args_};
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

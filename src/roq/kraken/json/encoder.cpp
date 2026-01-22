/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/json/encoder.hpp"

#include "roq/logging.hpp"

#include "roq/decimal.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/kraken/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace json {

// === HELPERS ===

namespace {
uint64_t pack_req_id(uint8_t user_id, uint64_t order_id) {
  assert(order_id <= ORDER_ID_MAX);
  return (static_cast<uint64_t>(user_id) << 56) | order_id;
}
}  // namespace

// === IMPLEMENTATION ===

// URL

// add-order

std::string_view Encoder::add_order_url(
    std::string &buffer, CreateOrder const &create_order, server::oms::Order const &order, std::string_view const &request_id) {
  auto side = map(create_order.side).template get<Side>();
  auto type = map(create_order.order_type).template get<OrderType>();
  auto reduce_only = false;
  buffer.clear();
  /*
  fmt::format_to(
      std::back_inserter(buffer),
      {
        "method" : "add_order",
        "params" : {
          "order_type" : "limit",
          "side" : "buy",
          "limit_price" : 26500.4,
          "order_userref" : 100054,
          "order_qty" : 1.2,
          "symbol" : "BTC/USD",
          "token" : "G38a1tGFzqGiUCmnegBcm8d4nfP3tytiNQz6tkCBYXY"
        },
        "req_id" : 123456789
      } create_order.symbol,
      side.as_raw_text(),
      type.as_raw_text(),
      Decimal{create_order.quantity, order.quantity_precision.precision},
      reduce_only);
  switch (create_order.order_type) {
    using enum roq::OrderType;
    case UNDEFINED:
      assert(false);
      break;
    case MARKET:
      assert(std::isnan(create_order.price));
      break;
    case LIMIT: {
      assert(!std::isnan(create_order.price));
      auto time_in_force = map(create_order.time_in_force, create_order.execution_instructions).template get<TimeInForce>();
      fmt::format_to(
          std::back_inserter(buffer),
          R"(timeInForce={}&)"
          R"(price={}&)"sv,
          time_in_force.as_raw_text(),
          Decimal{create_order.price, order.price_precision.precision});
      break;
    }
  }
  if (!std::isnan(create_order.stop_price)) {
    fmt::format_to(std::back_inserter(buffer), R"(stopPrice={}&)"sv, Decimal{create_order.stop_price, order.price_precision.precision});
  }
  fmt::format_to(
      std::back_inserter(buffer),
      R"(newClientOrderId={}&)"
      R"(recvWindow={})"sv,
      request_id,
      recv_window.count());
  */
  std::string_view result{std::data(buffer), std::size(buffer)};
  return result;
}

// amend-order

std::string_view Encoder::amend_order_url(
    std::string &buffer,
    roq::ModifyOrder const &modify_order,
    server::oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  /*
  auto side = map(order.side).template get<Side>();
  if (order_modify_full) {  // fapi
    auto quantity = std::isnan(modify_order.quantity) ? order.quantity : modify_order.quantity;
    auto price = std::isnan(modify_order.price) ? order.price : modify_order.price;
    fmt::format_to(std::back_inserter(buffer), R"(symbol={}&)"sv, order.symbol);
    if (!std::empty(order.external_order_id)) {
      fmt::format_to(std::back_inserter(buffer), R"(orderId={}&)"sv, order.external_order_id);
    }
    fmt::format_to(std::back_inserter(buffer), R"(origClientOrderId={}&)"sv, order.client_order_id);
    fmt::format_to(
        std::back_inserter(buffer),
        R"(side={}&)"
        R"(quantity={}&)"
        R"(price={}&)"
        R"(recvWindow={})"sv,
        side.as_raw_text(),
        Decimal{quantity, order.quantity_precision.precision},
        Decimal{price, order.price_precision.precision},
        recv_window.count());
  } else {  // dapi
    auto helper = [](auto value, auto last_value) {
      if (!std::isnan(value) && !utils::is_equal(value, last_value)) {
        return value;
      }
      return NaN;
    };
    auto quantity = helper(modify_order.quantity, order.quantity);
    auto price = helper(modify_order.price, order.price);
    if (!std::isnan(quantity) && std::isnan(price)) {
      fmt::format_to(std::back_inserter(buffer), R"(symbol={}&)"sv, order.symbol);
      if (!std::empty(order.external_order_id)) {
        fmt::format_to(std::back_inserter(buffer), R"(orderId={}&)"sv, order.external_order_id);
      }
      fmt::format_to(std::back_inserter(buffer), R"(origClientOrderId={}&)"sv, order.client_order_id);
      fmt::format_to(
          std::back_inserter(buffer),
          R"(side={}&)"
          R"(quantity={}&)"
          R"(recvWindow={})"sv,
          side.as_raw_text(),
          Decimal{modify_order.quantity, order.quantity_precision.precision},
          recv_window.count());
    } else if (std::isnan(quantity) && !std::isnan(price)) {
      fmt::format_to(std::back_inserter(buffer), R"(symbol={}&)"sv, order.symbol);
      if (!std::empty(order.external_order_id)) {
        fmt::format_to(std::back_inserter(buffer), R"(orderId={}&)"sv, order.external_order_id);
      }
      fmt::format_to(std::back_inserter(buffer), R"(origClientOrderId={}&)"sv, order.client_order_id);
      fmt::format_to(
          std::back_inserter(buffer),
          R"(side={}&)"
          R"(price={}&)"
          R"(recvWindow={})"sv,
          side.as_raw_text(),
          Decimal{modify_order.price, order.price_precision.precision},
          recv_window.count());
    } else {
      throw server::oms::Rejected{Origin::GATEWAY, Error::INVALID_REQUEST_ARGS, "Missing quantity or price"sv};
    }
  }
  */
  std::string_view result{std::data(buffer), std::size(buffer)};
  return result;
}

// cancel-order

std::string_view Encoder::cancel_order_url(
    std::string &buffer,
    roq::CancelOrder const &,
    server::oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  buffer.clear();
  fmt::format_to(std::back_inserter(buffer), R"(symbol={}&)"sv, order.symbol);
  if (!std::empty(order.external_order_id)) {
    fmt::format_to(std::back_inserter(buffer), R"(orderId={}&)"sv, order.external_order_id);
  }
  fmt::format_to(std::back_inserter(buffer), R"(origClientOrderId={}&)"sv, order.client_order_id);
  std::string_view result{std::data(buffer), std::size(buffer)};
  return result;
}

// JSON

// add-order

std::string_view Encoder::add_order_json(
    std::string &buffer, CreateOrder const &create_order, server::oms::Order const &order, std::string_view const &request_id, std::string_view const &token) {
  buffer.clear();
  auto side = map(create_order.side).template get<Side>();
  auto order_type = map(create_order.order_type).template get<OrderType>();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("method":"add_order",)"
      R"("params":{{)"
      R"("token":"{}",)"
      R"("cl_ord_id":"{}",)"
      R"("symbol":"{}",)"
      R"("side":"{}",)"
      R"("order_type":"{}",)"
      R"("order_qty":{})"sv,
      token,
      request_id,
      create_order.symbol,
      side.as_raw_text(),
      order_type.as_raw_text(),
      Decimal{create_order.quantity, order.quantity_precision.precision});
  if (create_order.time_in_force != roq::TimeInForce::UNDEFINED) {
    auto time_in_force = map(create_order.time_in_force).template get<TimeInForce>();
    fmt::format_to(std::back_inserter(buffer), R"(,"time_in_force":"{}")"sv, time_in_force.as_raw_text());
  }
  if (!std::isnan(create_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"limit_price":{})"sv, Decimal{create_order.price, order.price_precision.precision});
  }
  auto req_id = pack_req_id(order.user_id, order.order_id);
  fmt::format_to(
      std::back_inserter(buffer),
      R"(}},)"
      R"("req_id":{})"
      R"(}})"sv,
      req_id);
  std::string_view result{std::data(buffer), std::size(buffer)};
  return result;
}

// amend-order

std::string_view Encoder::amend_order_json(
    std::string &buffer,
    roq::ModifyOrder const &modify_order,
    server::oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id,
    std::string_view const &token) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("method":"amend_order",)"
      R"("params":{{)"
      R"("token":"{}",)"
      R"("cl_ord_id":"{}")"sv,
      token,
      order.client_order_id);
  if (!std::isnan(modify_order.quantity)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"order_qty":{})"sv, Decimal{modify_order.quantity, order.quantity_precision.precision});
  }
  if (!std::isnan(modify_order.price)) {
    fmt::format_to(std::back_inserter(buffer), R"(,"limit_price":{})"sv, Decimal{modify_order.price, order.price_precision.precision});
  }
  auto req_id = pack_req_id(order.user_id, order.order_id);
  fmt::format_to(
      std::back_inserter(buffer),
      R"(}},)"
      R"("req_id":{})"
      R"(}})"sv,
      req_id);
  std::string_view result{std::data(buffer), std::size(buffer)};
  return result;
}

// cancel-order

std::string_view Encoder::cancel_order_json(
    std::string &buffer,
    roq::CancelOrder const &,
    server::oms::Order const &order,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id,
    std::string_view const &token) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("method":"cancel_order",)"
      R"("params":{{)"
      R"("token":"{}",)"sv,
      token);
  if (std::empty(order.external_order_id)) {
    fmt::format_to(std::back_inserter(buffer), R"("cl_ord_id":["{}"])"sv, order.client_order_id);
  } else {
    fmt::format_to(std::back_inserter(buffer), R"("order_id":["{}"])"sv, order.external_order_id);
  }
  auto req_id = pack_req_id(order.user_id, order.order_id);
  fmt::format_to(
      std::back_inserter(buffer),
      R"(}},)"
      R"("req_id":{})"
      R"(}})"sv,
      req_id);
  std::string_view result{std::data(buffer), std::size(buffer)};
  return result;
}

// cancel-all

std::string_view Encoder::cancel_all_json(
    std::string &buffer, roq::CancelAllOrders const &, [[maybe_unused]] std::string_view const &request_id, std::string_view const &token) {
  buffer.clear();
  fmt::format_to(
      std::back_inserter(buffer),
      R"({{)"
      R"("method":"cancel_all",)"
      R"("params":{{)"
      R"("token":"{}")"
      R"(}},)"
      R"("req_id":123)"
      R"(}})"sv,
      token);
  std::string_view result{std::data(buffer), std::size(buffer)};
  return result;
}

// helpers

std::pair<uint8_t, uint64_t> Encoder::unpack_req_id(uint64_t req_id) {
  return {
      static_cast<uint8_t>(req_id >> 56),
      req_id & ((uint64_t{1} << 48) - 1),
  };
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include "roq/cancel_all_orders.hpp"
#include "roq/cancel_order.hpp"
#include "roq/create_order.hpp"
#include "roq/modify_order.hpp"

#include "roq/server/oms/order.hpp"
#include "roq/server/oms/ref_data.hpp"

namespace roq {
namespace kraken {
namespace json {

struct Encoder final {
  // URL

  // order-place

  static std::string_view add_order_url(
      std::string &buffer, CreateOrder const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);

  // amend-order

  static std::string_view amend_order_url(
      std::string &buffer,
      roq::ModifyOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  // cancel-order

  static std::string_view cancel_order_url(
      std::string &buffer,
      roq::CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  // JSON

  // add-order

  static std::string_view add_order_json(
      std::string &buffer,
      CreateOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &token);

  // amend-order

  static std::string_view amend_order_json(
      std::string &buffer,
      roq::ModifyOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id,
      std::string_view const &token);

  // cancel-order

  static std::string_view cancel_order_json(
      std::string &buffer,
      roq::CancelOrder const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id,
      std::string_view const &token);

  // cancel-all

  static std::string_view cancel_all_json(std::string &buffer, roq::CancelAllOrders const &, std::string_view const &request_id, std::string_view const &token);

  // helpers

  static std::pair<uint8_t, uint64_t> unpack_req_id(uint64_t req_id);
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

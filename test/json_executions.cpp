/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Executions;

TEST_CASE("empty", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"snapshot",)"
                 R"("data":[],)"
                 R"("sequence":1)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "executions"sv);
    CHECK(obj.type == json::Type::SNAPSHOT);
    REQUIRE(std::empty(obj.data));
    CHECK(obj.sequence == 1);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("pending_new", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("order_id":"OVN7VD-7VYHS-APBKHA",)"
                 R"("symbol":"BTC/USDT",)"
                 R"("order_qty":0.00010000,)"
                 R"("cum_cost":0.00000,)"
                 R"("time_in_force":"GTC",)"
                 R"("exec_type":"pending_new",)"
                 R"("side":"buy",)"
                 R"("order_type":"limit",)"
                 R"("cl_ord_id":"b17c7bb34d000200",)"
                 R"("limit_price_type":"static",)"
                 R"("limit_price":80000.0,)"
                 R"("stop_price":0.0,)"
                 R"("order_status":"pending_new",)"
                 R"("fee_usd_equiv":0.0000,)"
                 R"("fee_ccy_pref":"fciq",)"
                 R"("timestamp":"2026-01-22T06:06:19.833306Z")"
                 R"(})"
                 R"(],)"
                 R"("sequence":2)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "executions"sv);
    CHECK(obj.type == json::Type::UPDATE);
    REQUIRE(std::size(obj.data) == 1);
    //
    CHECK(obj.sequence == 2);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("new", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("timestamp":"2026-01-22T06:06:19.833306Z",)"
                 R"("order_status":"new",)"
                 R"("exec_type":"new",)"
                 R"("cl_ord_id":"b17c7bb34d000200",)"
                 R"("order_id":"OVN7VD-7VYHS-APBKHA")"
                 R"(})"
                 R"(],)"
                 R"("sequence":3)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "executions"sv);
    CHECK(obj.type == json::Type::UPDATE);
    REQUIRE(std::size(obj.data) == 1);
    //
    CHECK(obj.sequence == 3);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("amended_price", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("timestamp":"2026-01-22T06:06:27.552281Z",)"
                 R"("exec_type":"amended",)"
                 R"("order_status":"new",)"
                 R"("cum_qty":0.00000000,)"
                 R"("reason":"User requested",)"
                 R"("amended":true,)"
                 R"("order_qty":0.00010000,)"
                 R"("limit_price":81818.0,)"
                 R"("limit_price_type":"static",)"
                 R"("cl_ord_id":"b17c7bb34d000200",)"
                 R"("order_id":"OVN7VD-7VYHS-APBKHA")"
                 R"(})"
                 R"(],)"
                 R"("sequence":4)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "executions"sv);
    CHECK(obj.type == json::Type::UPDATE);
    REQUIRE(std::size(obj.data) == 1);
    //
    CHECK(obj.sequence == 4);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("amended_quantity", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("timestamp":"2026-01-22T06:06:27.552281Z",)"
                 R"("exec_type":"amended",)"
                 R"("order_status":"new",)"
                 R"("cum_qty":0.00000000,)"
                 R"("reason":"User requested",)"
                 R"("amended":true,)"
                 R"("order_qty":0.00005000,)"
                 R"("limit_price":81818.0,)"
                 R"("limit_price_type":"static",)"
                 R"("cl_ord_id":"b17c7bb34d000200",)"
                 R"("order_id":"OVN7VD-7VYHS-APBKHA")"
                 R"(})"
                 R"(],)"
                 R"("sequence":5)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "executions"sv);
    CHECK(obj.type == json::Type::UPDATE);
    REQUIRE(std::size(obj.data) == 1);
    //
    CHECK(obj.sequence == 5);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("canceled", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("timestamp":"2026-01-22T06:06:36.039852Z",)"
                 R"("order_status":"canceled",)"
                 R"("exec_type":"canceled",)"
                 R"("cum_qty":0.00000000,)"
                 R"("cum_cost":0.00000,)"
                 R"("fee_usd_equiv":0.0000,)"
                 R"("avg_price":0.0,)"
                 R"("cl_ord_id":"b17c7bb34d000200",)"
                 R"("cancel_reason":"User requested",)"
                 R"("reason":"User requested",)"
                 R"("order_id":"OVN7VD-7VYHS-APBKHA")"
                 R"(})"
                 R"(],)"
                 R"("sequence":6)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "executions"sv);
    CHECK(obj.type == json::Type::UPDATE);
    REQUIRE(std::size(obj.data) == 1);
    //
    CHECK(obj.sequence == 6);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

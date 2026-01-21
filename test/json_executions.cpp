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
                 R"("order_id":"O5ORI4-WNTKR-CHOYU2",)"
                 R"("symbol":"BTC/USDT",)"
                 R"("order_qty":0.00010000,)"
                 R"("cum_cost":0.00000,)"
                 R"("time_in_force":"GTC",)"
                 R"("exec_type":"pending_new",)"
                 R"("side":"buy",)"
                 R"("order_type":"limit",)"
                 R"("cl_ord_id":"b6755a8d4d000200",)"
                 R"("limit_price_type":"static",)"
                 R"("limit_price":8000.0,)"
                 R"("stop_price":0.0,)"
                 R"("order_status":"pending_new",)"
                 R"("fee_usd_equiv":0.0000,)"
                 R"("fee_ccy_pref":"fciq",)"
                 R"("timestamp":"2026-01-21T12:20:06.637700Z")"
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

TEST_CASE("canceled", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("timestamp":"2026-01-21T14:19:26.499881Z",)"
                 R"("order_status":"canceled",)"
                 R"("exec_type":"canceled",)"
                 R"("cum_qty":0.00000000,)"
                 R"("cum_cost":0.00000,)"
                 R"("fee_usd_equiv":0.0000,)"
                 R"("avg_price":0.0,)"
                 R"("cl_ord_id":"82909e914d000200",)"
                 R"("cancel_reason":"User requested",)"
                 R"("reason":"User requested",)"
                 R"("order_id":"OLASKX-JMREK-KGIHQT")"
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

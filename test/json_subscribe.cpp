/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using value_type = protocol::json::Subscribe;

TEST_CASE("error", "[json_subscribe]") {
  auto message = R"({)"
                 R"("error":"Unsupported field: 'channel' for the given msg type: subscribe",)"
                 R"("method":"subscribe",)"
                 R"("success":false,)"
                 R"("symbol":"ETH/USDC",)"
                 R"("time_in":"2026-01-19T06:39:36.354004Z",)"
                 R"("time_out":"2026-01-19T06:39:36.354054Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error == "Unsupported field: 'channel' for the given msg type: subscribe"sv);
    CHECK(obj.method == "subscribe"sv);
    CHECK(obj.success == false);
    CHECK(obj.symbol == "ETH/USDC"sv);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("instrument", "[json_subscribe]") {
  auto message = R"({)"
                 R"("method":"subscribe",)"
                 R"("result":{)"
                 R"("channel":"instrument",)"
                 R"("snapshot":true,)"
                 R"("warnings":[)"
                 R"("tick_size is deprecated, use price_increment")"
                 R"(])"
                 R"(},)"
                 R"("success":true,)"
                 R"("time_in":"2026-01-19T10:05:33.672332Z",)"
                 R"("time_out":"2026-01-19T10:05:33.672369Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.result.channel == "instrument"sv);
    CHECK(obj.result.snapshot == true);
    REQUIRE(std::size(obj.result.warnings) == 1);
    CHECK(obj.result.warnings[0] == "tick_size is deprecated, use price_increment"sv);
    CHECK(obj.success == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("ticker", "[json_subscribe]") {
  auto message = R"({)"
                 R"("method":"subscribe",)"
                 R"("result":{)"
                 R"("channel":"ticker",)"
                 R"("event_trigger":"trades",)"
                 R"("snapshot":true,)"
                 R"("symbol":"ETH/USDC")"
                 R"(},)"
                 R"("success":true,)"
                 R"("time_in":"2026-01-19T13:03:41.074343Z",)"
                 R"("time_out":"2026-01-19T13:03:41.074403Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.result.channel == "ticker"sv);
    CHECK(obj.result.event_trigger == "trades"sv);
    CHECK(obj.result.snapshot == true);
    CHECK(obj.result.symbol == "ETH/USDC"sv);
    CHECK(std::empty(obj.result.warnings));
    CHECK(obj.success == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("executions", "[json_subscribe]") {
  auto message = R"({)"
                 R"("method":"subscribe",)"
                 R"("result":{)"
                 R"("channel":"executions",)"
                 R"("maxratecount":125,)"
                 R"("snap_orders":true,)"
                 R"("snap_trades":true,)"
                 R"("snapshot":true,)"
                 R"("warnings":[)"
                 R"("cancel_reason is deprecated, use reason",)"
                 R"("stop_price is deprecated, use triggers.price",)"
                 R"("trigger is deprecated use triggers.reference",)"
                 R"("triggered_price is deprecated use triggers.last_price")"
                 R"(])"
                 R"(},)"
                 R"("success":true,)"
                 R"("time_in":"2026-01-20T14:03:46.011991Z",)"
                 R"("time_out":"2026-01-20T14:03:46.018146Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.result.channel == "executions"sv);
    CHECK(obj.result.maxratecount == 125);
    CHECK(obj.result.snap_orders == true);
    CHECK(obj.result.snap_trades == true);
    CHECK(obj.result.snapshot == true);
    CHECK(std::size(obj.result.warnings) == 4);
    CHECK(obj.success == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

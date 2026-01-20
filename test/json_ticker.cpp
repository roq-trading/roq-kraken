/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Ticker;

TEST_CASE("snapshot", "[json_ticker]") {
  auto message = R"({)"
                 R"("channel":"ticker",)"
                 R"("type":"snapshot",)"
                 R"("data":[{)"
                 R"("symbol":"ETH/USDC",)"
                 R"("bid":3207.04,)"
                 R"("bid_qty":0.13280000,)"
                 R"("ask":3207.71,)"
                 R"("ask_qty":0.13280000,)"
                 R"("last":3208.47,)"
                 R"("volume":2173.29284504,)"
                 R"("vwap":3243.14,)"
                 R"("low":3177.79,)"
                 R"("high":3367.99,)"
                 R"("change":-119.77,)"
                 R"("change_pct":-3.60,)"
                 R"("volume_usd":6968011.52,)"
                 R"("timestamp":"2026-01-19T14:45:24.534338Z")"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "ticker"sv);
    CHECK(obj.type == json::Type::SNAPSHOT);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("update", "[json_ticker]") {
  auto message = R"({)"
                 R"("channel":"ticker",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("symbol":"ETH/USD",)"
                 R"("bid":3215.88,)"
                 R"("bid_qty":3.64088999,)"
                 R"("ask":3215.89,)"
                 R"("ask_qty":70.24300000,)"
                 R"("last":3215.88,)"
                 R"("volume":25414.76429817,)"
                 R"("vwap":3240.91,)"
                 R"("low":3174.44,)"
                 R"("high":3367.00,)"
                 R"("change":-109.77,)"
                 R"("change_pct":-3.30,)"
                 R"("volume_usd":81730832.21,)"
                 R"("timestamp":"2026-01-19T15:05:26.339692Z")"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "ticker"sv);
    CHECK(obj.type == json::Type::UPDATE);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

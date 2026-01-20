/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Instrument;

// note! truncated
TEST_CASE("snapshot", "[json_instrument]") {
  auto message = R"({)"
                 R"("channel":"instrument",)"
                 R"("type":"snapshot",)"
                 R"("data":{)"
                 R"("assets":[{)"
                 R"("id":"USD",)"
                 R"("status":"enabled",)"
                 R"("precision":4,)"
                 R"("precision_display":2,)"
                 R"("borrowable":true,)"
                 R"("collateral_value":1.000,)"
                 R"("class":"currency",)"
                 R"("margin_rate":0.050000)"
                 R"(},{)"
                 R"("id":"EUR",)"
                 R"("status":"enabled",)"
                 R"("precision":4,)"
                 R"("precision_display":2,)"
                 R"("borrowable":true,)"
                 R"("collateral_value":1.000,)"
                 R"("class":"currency",)"
                 R"("margin_rate":0.025000)"
                 R"(},{)"
                 R"("id":"USDT.M",)"
                 R"("status":"enabled",)"
                 R"("precision":8,)"
                 R"("precision_display":4,)"
                 R"("borrowable":false,)"
                 R"("collateral_value":0.000,)"
                 R"("class":"currency")"
                 R"(},{)"
                 R"("id":"USDC.M",)"
                 R"("status":"enabled",)"
                 R"("precision":8,)"
                 R"("precision_display":4,)"
                 R"("borrowable":false,)"
                 R"("collateral_value":0.000,)"
                 R"("class":"currency")"
                 R"(})"
                 R"(],)"
                 R"("pairs":[{)"
                 R"("symbol":"EUR/USD",)"
                 R"("base":"EUR",)"
                 R"("quote":"USD",)"
                 R"("status":"online",)"
                 R"("qty_precision":8,)"
                 R"("qty_increment":0.00000001,)"
                 R"("price_precision":5,)"
                 R"("cost_precision":5,)"
                 R"("marginable":false,)"
                 R"("has_index":true,)"
                 R"("ws_display_price_precision":5,)"
                 R"("cost_min":0.50,)"
                 R"("tick_size":0.00001,)"
                 R"("price_increment":0.00001,)"
                 R"("qty_min":0.50000000)"
                 R"(},{)"
                 R"("symbol":"GBP/USD",)"
                 R"("base":"GBP",)"
                 R"("quote":"USD",)"
                 R"("status":"online",)"
                 R"("qty_precision":8,)"
                 R"("qty_increment":0.00000001,)"
                 R"("price_precision":5,)"
                 R"("cost_precision":5,)"
                 R"("marginable":false,)"
                 R"("has_index":true,)"
                 R"("ws_display_price_precision":5,)"
                 R"("cost_min":0.50,)"
                 R"("tick_size":0.00001,)"
                 R"("price_increment":0.00001,)"
                 R"("qty_min":5.00000000)"
                 R"(},{)"
                 R"("symbol":"VET/USDT",)"
                 R"("base":"VET",)"
                 R"("quote":"USDT",)"
                 R"("status":"post_only",)"
                 R"("qty_precision":5,)"
                 R"("qty_increment":0.00001,)"
                 R"("price_precision":6,)"
                 R"("cost_precision":5,)"
                 R"("marginable":false,)"
                 R"("has_index":false,)"
                 R"("ws_display_price_precision":6,)"
                 R"("cost_min":0.5000,)"
                 R"("tick_size":0.000001,)"
                 R"("price_increment":0.000001,)"
                 R"("qty_min":200.00000)"
                 R"(},{)"
                 R"("symbol":"VET/USDC",)"
                 R"("base":"VET",)"
                 R"("quote":"USDC",)"
                 R"("status":"post_only",)"
                 R"("qty_precision":5,)"
                 R"("qty_increment":0.00001,)"
                 R"("price_precision":6,)"
                 R"("cost_precision":5,)"
                 R"("marginable":false,)"
                 R"("has_index":false,)"
                 R"("ws_display_price_precision":6,)"
                 R"("cost_min":0.5000,)"
                 R"("tick_size":0.000001,)"
                 R"("price_increment":0.000001,)"
                 R"("qty_min":200.00000)"
                 R"(})"
                 R"(])"
                 R"(})"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "instrument"sv);
    CHECK(obj.type == json::Type::SNAPSHOT);
    REQUIRE(std::size(obj.data.assets) == 4);
    auto &a0 = obj.data.assets[0];
    CHECK(a0.id == "USD"sv);
    CHECK(a0.status == "enabled"sv);  // XXX FIXME TODO enum?
    CHECK(a0.precision == 4);
    CHECK(a0.precision_display == 2);
    CHECK(a0.borrowable == true);
    CHECK(a0.collateral_value == 1.0_a);
    CHECK(a0.class_ == "currency"sv);
    CHECK(a0.margin_rate == 0.05_a);
    REQUIRE(std::size(obj.data.pairs) == 4);
    auto &p0 = obj.data.pairs[3];
    CHECK(p0.symbol == "VET/USDC"sv);
    CHECK(p0.base == "VET"sv);
    CHECK(p0.quote == "USDC"sv);
    CHECK(p0.status == json::PairsStatus::POST_ONLY);
    CHECK(p0.qty_precision == 5);
    CHECK(p0.qty_increment == 0.00001_a);
    CHECK(p0.price_precision == 6);
    CHECK(p0.cost_precision == 5);
    CHECK(p0.marginable == false);
    CHECK(p0.has_index == false);
    CHECK(p0.ws_display_price_precision == 6);
    CHECK(p0.cost_min == 0.5_a);
    CHECK(p0.tick_size == 0.000001_a);
    CHECK(p0.price_increment == 0.000001_a);
    CHECK(p0.qty_min == 200.0_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 65536, 1);
}

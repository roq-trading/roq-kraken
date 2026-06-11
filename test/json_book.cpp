/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::Book;

TEST_CASE("snapshot", "[json_book]") {
  auto message = R"({)"
                 R"("channel":"book",)"
                 R"("type":"snapshot",)"
                 R"("data":[{)"
                 R"("symbol":"ETH/USDT",)"
                 R"("bids":[)"
                 R"({"price":3215.98,"qty":0.43374000},)"
                 R"({"price":3215.88,"qty":1.55563481},)"
                 R"({"price":3215.68,"qty":0.11390000},)"
                 R"({"price":3215.60,"qty":0.31899455},)"
                 R"({"price":3215.46,"qty":0.31577998},)"
                 R"({"price":3215.41,"qty":1.11400000},)"
                 R"({"price":3215.40,"qty":0.75897979},)"
                 R"({"price":3215.36,"qty":0.34384896},)"
                 R"({"price":3215.35,"qty":0.11390000},)"
                 R"({"price":3215.33,"qty":5.00183483})"
                 R"(],)"
                 R"("asks":[)"
                 R"({"price":3216.17,"qty":1.57368901},)"
                 R"({"price":3216.18,"qty":3.10841504},)"
                 R"({"price":3216.26,"qty":5.00183483},)"
                 R"({"price":3216.35,"qty":1.68100000},)"
                 R"({"price":3216.53,"qty":0.11390000},)"
                 R"({"price":3216.61,"qty":6.05150000},)"
                 R"({"price":3216.66,"qty":0.34384896},)"
                 R"({"price":3216.67,"qty":0.32189137},)"
                 R"({"price":3216.88,"qty":0.11390000},)"
                 R"({"price":3216.89,"qty":0.02000000})"
                 R"(],)"
                 R"("checksum":1875479340,)"
                 R"("timestamp":"2026-01-19T15:56:44.723626Z")"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "book"sv);
    CHECK(obj.type == protocol::json::Type::SNAPSHOT);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}

TEST_CASE("update", "[json_book]") {
  auto message = R"({)"
                 R"("channel":"book",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("symbol":"ETH/USDC",)"
                 R"("bids":[],)"
                 R"("asks":[)"
                 R"({"price":3213.71,"qty":0.00000000},)"
                 R"({"price":3214.99,"qty":5.11001334}],)"
                 R"("checksum":2768700782,)"
                 R"("timestamp":"2026-01-19T16:15:30.114195Z")"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "book"sv);
    CHECK(obj.type == protocol::json::Type::UPDATE);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}

// note! repeated price range
TEST_CASE("strange", "[json_book]") {
  auto message = R"({)"
                 R"("channel":"book",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("symbol":"BTC/USDT",)"
                 R"("bids":[)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171536},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000},)"
                 R"({"price":89773.7,"qty":0.00171534},)"
                 R"({"price":89773.7,"qty":0.00000000},)"
                 R"({"price":89723.4,"qty":0.01320000})"
                 R"(],)"
                 R"("asks":[],)"
                 R"("checksum":1773535742,)"
                 R"("timestamp":"2026-01-23T15:49:41.026930Z")"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "book"sv);
    CHECK(obj.type == protocol::json::Type::UPDATE);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}

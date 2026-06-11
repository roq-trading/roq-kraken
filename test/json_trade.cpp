/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::Trade;

TEST_CASE("update", "[json_trade]") {
  auto message = R"({)"
                 R"("channel":"trade",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("symbol":"ETH/USD",)"
                 R"("side":"buy",)"
                 R"("price":3213.89,)"
                 R"("qty":0.02315574,)"
                 R"("ord_type":"market",)"
                 R"("trade_id":59874571,)"
                 R"("timestamp":"2026-01-19T15:36:48.413515Z")"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "trade"sv);
    CHECK(obj.type == protocol::json::Type::UPDATE);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

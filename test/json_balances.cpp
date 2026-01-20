/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Balances;

TEST_CASE("empty", "[json_balances]") {
  auto message = R"({)"
                 R"("channel":"balances",)"
                 R"("type":"snapshot",)"
                 R"("data":[{)"
                 R"("asset":"USDT",)"
                 R"("asset_class":"currency",)"
                 R"("wallets":[{)"
                 R"("type":"spot",)"
                 R"("id":"main",)"
                 R"("balance":100.00000000)"
                 R"(})"
                 R"(],)"
                 R"("balance":100.00000000)"
                 R"(})"
                 R"(],)"
                 R"("sequence":1)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "balances"sv);
    CHECK(obj.type == json::Type::SNAPSHOT);
    REQUIRE(std::size(obj.data) == 1);
    auto &d0 = obj.data[0];
    CHECK(d0.asset == "USDT"sv);
    CHECK(d0.asset_class == "currency"sv);
    REQUIRE(std::size(d0.wallets) == 1);
    auto &w0 = d0.wallets[0];
    CHECK(w0.type == "spot"sv);
    CHECK(w0.id == "main"sv);
    CHECK(w0.balance == 100.0_a);
    CHECK(d0.balance == 100.0_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}

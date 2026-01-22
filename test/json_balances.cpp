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

TEST_CASE("leg_1", "[json_balances]") {
  auto message = R"({)"
                 R"("channel":"balances",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("ledger_id":"LMP34Z-UOZMC-XNSVXD",)"
                 R"("ref_id":"TISWUK-U5KR2-4BYNCI",)"
                 R"("timestamp":"2026-01-22T10:17:55.214460Z",)"
                 R"("type":"trade",)"
                 R"("asset":"BTC",)"
                 R"("asset_class":"currency",)"
                 R"("category":"trade",)"
                 R"("wallet_type":"spot",)"
                 R"("wallet_id":"main",)"
                 R"("amount":-0.0001000000,)"
                 R"("fee":0.0000000000,)"
                 R"("balance":0.0000000000)"
                 R"(})"
                 R"(],)"
                 R"("sequence":2)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "balances"sv);
    CHECK(obj.type == json::Type::UPDATE);
    REQUIRE(std::size(obj.data) == 1);
    auto &d0 = obj.data[0];
    CHECK(d0.ledger_id == "LMP34Z-UOZMC-XNSVXD"sv);
    CHECK(d0.ref_id == "TISWUK-U5KR2-4BYNCI"sv);
    // timestamp
    CHECK(d0.type == "trade"sv);
    CHECK(d0.asset == "BTC"sv);
    CHECK(d0.asset_class == "currency"sv);
    CHECK(d0.category == "trade"sv);
    CHECK(d0.wallet_type == "spot"sv);
    CHECK(d0.wallet_id == "main"sv);
    CHECK(d0.amount == -0.0001_a);
    CHECK(d0.fee == 0.0_a);
    CHECK(d0.balance == 0.0_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}

TEST_CASE("leg_2", "[json_balances]") {
  auto message = R"({)"
                 R"("channel":"balances",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("ledger_id":"LKZ5PE-XN427-KNE4XP",)"
                 R"("ref_id":"TISWUK-U5KR2-4BYNCI",)"
                 R"("timestamp":"2026-01-22T10:17:55.214460Z",)"
                 R"("type":"trade",)"
                 R"("asset":"USDT",)"
                 R"("asset_class":"currency",)"
                 R"("category":"trade",)"
                 R"("wallet_type":"spot",)"
                 R"("wallet_id":"main",)"
                 R"("amount":9.00500000,)"
                 R"("fee":0.02251000,)"
                 R"("balance":99.89293000)"
                 R"(})"
                 R"(],)"
                 R"("sequence":3)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "balances"sv);
    CHECK(obj.type == json::Type::UPDATE);
    REQUIRE(std::size(obj.data) == 1);
    auto &d0 = obj.data[0];
    CHECK(d0.ledger_id == "LKZ5PE-XN427-KNE4XP"sv);
    CHECK(d0.ref_id == "TISWUK-U5KR2-4BYNCI"sv);
    // timestamp
    CHECK(d0.type == "trade"sv);
    CHECK(d0.asset == "USDT"sv);
    CHECK(d0.asset_class == "currency"sv);
    CHECK(d0.category == "trade"sv);
    CHECK(d0.wallet_type == "spot"sv);
    CHECK(d0.wallet_id == "main"sv);
    CHECK(d0.amount == 9.005_a);
    CHECK(d0.fee == 0.02251_a);
    CHECK(d0.balance == 99.89293_a);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 2);
}

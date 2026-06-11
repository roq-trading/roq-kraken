/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/protocol/json/balance_ack.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using value_type = protocol::json::BalanceAck;

TEST_CASE("simple", "[json_balance_ack]") {
  auto const message = R"({)"
                       R"("error":[],)"
                       R"("result":{)"
                       R"("USDT":"100.00000000")"
                       R"(})"
                       R"(})"sv;
  auto helper = [&](value_type &obj) {
    CHECK(std::empty(obj.error));
    REQUIRE(std::size(obj.result) == 1);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

TEST_CASE("simple_2", "[json_balance_ack]") {
  auto const message = R"({)"
                       R"("error":[],)"
                       R"("result":{)"
                       R"("USDT":"90.96791000",)"
                       R"("XXBT":"0.0001000000")"
                       R"(})"
                       R"(})"sv;
  auto helper = [&](value_type &obj) {
    CHECK(std::empty(obj.error));
    REQUIRE(std::size(obj.result) == 2);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/protocol/json/trade_balance_ack.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::TradeBalanceAck;

TEST_CASE("simple", "[json_trade_balance_ack]") {
  auto const message = R"({)"
                       R"("error":[],)"
                       R"("result":{)"
                       R"("eb":"99.7418",)"
                       R"("tb":"99.1982",)"
                       R"("m":"0.0000",)"
                       R"("uv":"0.0000",)"
                       R"("n":"0.0000",)"
                       R"("c":"0.0000",)"
                       R"("v":"0.0000",)"
                       R"("e":"99.1982",)"
                       R"("mf":"99.1982",)"
                       R"("mfo":"90.3483")"
                       R"(})"
                       R"(})"sv;
  auto helper = [&](value_type &obj) {
    CHECK(std::empty(obj.error));
    CHECK(obj.result.equivalent_balance == 99.7418_a);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

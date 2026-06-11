/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/protocol/json/open_orders_ack.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using value_type = protocol::json::OpenOrdersAck;

TEST_CASE("simple", "[json_open_orders_ack]") {
  auto message = R"({)"
                 R"("error":[],)"
                 R"("result":{)"
                 R"("open":{)"
                 R"("OFIYM3-N4EEJ-2O6UKI":{)"
                 R"("refid":null,)"
                 R"("userref":null,)"
                 R"("cl_ord_id":"aff1f7b04d000200",)"
                 R"("status":"open",)"
                 R"("opentm":1769057759.97471,)"
                 R"("starttm":0,)"
                 R"("expiretm":0,)"
                 R"("descr":{)"
                 R"("pair":"XBTUSDT",)"
                 R"("aclass":"forex",)"
                 R"("type":"buy",)"
                 R"("ordertype":"limit",)"
                 R"("price":"81818.0",)"
                 R"("price2":"0",)"
                 R"("leverage":"none",)"
                 R"("order":"buy 0.00010000 XBTUSDT @ limit 81818.0",)"
                 R"("close":"")"
                 R"(},)"
                 R"("vol":"0.00010000",)"
                 R"("vol_exec":"0.00000000",)"
                 R"("cost":"0.00000",)"
                 R"("fee":"0.00000",)"
                 R"("price":"0.00000",)"
                 R"("stopprice":"0.00000",)"
                 R"("limitprice":"0.00000",)"
                 R"("misc":"amended",)"
                 R"("oflags":"fciq",)"
                 R"("time_in_force":"gtc")"
                 R"(},)"
                 R"("OHSXMA-KHS6O-KYAGNK":{)"
                 R"("refid":null,)"
                 R"("userref":null,)"
                 R"("cl_ord_id":"9b80dfb04d000200",)"
                 R"("status":"open",)"
                 R"("opentm":1769057598.033432,)"
                 R"("starttm":0,)"
                 R"("expiretm":0,)"
                 R"("descr":{)"
                 R"("pair":"XBTUSDT",)"
                 R"("aclass":"forex",)"
                 R"("type":"buy",)"
                 R"("ordertype":"limit",)"
                 R"("price":"80000.0",)"
                 R"("price2":"0",)"
                 R"("leverage":"none",)"
                 R"("order":"buy 0.00010000 XBTUSDT @ limit 80000.0",)"
                 R"("close":"")"
                 R"(},)"
                 R"("vol":"0.00010000",)"
                 R"("vol_exec":"0.00000000",)"
                 R"("cost":"0.00000",)"
                 R"("fee":"0.00000",)"
                 R"("price":"0.00000",)"
                 R"("stopprice":"0.00000",)"
                 R"("limitprice":"0.00000",)"
                 R"("misc":"",)"
                 R"("oflags":"fciq",)"
                 R"("time_in_force":"gtc")"
                 R"(})"
                 R"(})"
                 R"(})"
                 R"(})"sv;
  auto helper = [&](value_type &obj) {
    CHECK(std::empty(obj.error));
    REQUIRE(std::size(obj.result.open) == 2);
    auto &o0 = obj.result.open[0];
    CHECK(o0.KEY == "OFIYM3-N4EEJ-2O6UKI"sv);
    auto &o1 = obj.result.open[1];
    CHECK(o1.KEY == "OHSXMA-KHS6O-KYAGNK"sv);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

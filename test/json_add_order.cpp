/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::AddOrder;

TEST_CASE("failure", "[json_add_order]") {
  auto message = R"({)"
                 R"("error":"EGeneral:Invalid arguments:volume minimum not met",)"
                 R"("method":"add_order",)"
                 R"("success":false,)"
                 R"("time_in":"2026-01-21T14:24:10.817739Z",)"
                 R"("time_out":"2026-01-21T14:24:10.819873Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error == "EGeneral:Invalid arguments:volume minimum not met"sv);
    CHECK(obj.method == "add_order"sv);
    CHECK(obj.success == false);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("success", "[json_add_order]") {
  auto message = R"({)"
                 R"("method":"add_order",)"
                 R"("result":{)"
                 R"("cl_ord_id":"3178d58d4d000200",)"
                 R"("order_id":"OXZPAJ-ONWQU-KMAMPL")"
                 R"(},)"
                 R"("success":true,)"
                 R"("time_in":"2026-01-21T12:33:34.457870Z",)"
                 R"("time_out":"2026-01-21T12:33:34.460575Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == "add_order"sv);
    CHECK(obj.result.cl_ord_id == "3178d58d4d000200"sv);
    CHECK(obj.result.order_id == "OXZPAJ-ONWQU-KMAMPL"sv);
    CHECK(obj.success == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

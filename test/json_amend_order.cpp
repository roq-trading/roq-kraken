/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::AmendOrder;

TEST_CASE("failure", "[json_amend_order]") {
  auto message = R"({)"
                 R"("error":"EOrder:Unknown order",)"
                 R"("method":"amend_order",)"
                 R"("success":false,)"
                 R"("time_in":"2026-01-22T05:02:09.671470Z",)"
                 R"("time_out":"2026-01-22T05:02:09.672399Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error == "EOrder:Unknown order"sv);
    CHECK(obj.method == "amend_order"sv);
    CHECK(obj.success == false);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("success", "[json_amend_order]") {
  auto message = R"({)"
                 R"("method":"amend_order",)"
                 R"("result":{)"
                 R"("amend_id":"TTB6D6-Q4HHW-DQ5FQW",)"
                 R"("cl_ord_id":"aff1f7b04d000200")"
                 R"(},)"
                 R"("success":true,)"
                 R"("time_in":"2026-01-22T04:56:03.261354Z",)"
                 R"("time_out":"2026-01-22T04:56:03.264302Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == "amend_order"sv);
    CHECK(obj.result.amend_id == "TTB6D6-Q4HHW-DQ5FQW"sv);
    CHECK(obj.result.cl_ord_id == "aff1f7b04d000200"sv);
    CHECK(obj.success == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

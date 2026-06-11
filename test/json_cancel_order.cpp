/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::CancelOrder;

TEST_CASE("failure", "[json_cancel_order]") {
  auto message = R"({)"
                 R"("error":"EOrder:Unknown order",)"
                 R"("method":"cancel_order",)"
                 R"("success":false,)"
                 R"("time_in":"2026-01-21T14:28:12.664421Z",)"
                 R"("time_out":"2026-01-21T14:28:12.666548Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error == "EOrder:Unknown order"sv);
    CHECK(obj.method == "cancel_order"sv);
    CHECK(obj.success == false);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

TEST_CASE("success", "[json_cancel_order]") {
  auto message = R"({)"
                 R"("method":"cancel_order",)"
                 R"("result":{)"
                 R"("cl_ord_id":"6a3260914d000200")"
                 R"(},)"
                 R"("success":true,)"
                 R"("time_in":"2026-01-21T14:12:39.343493Z",)"
                 R"("time_out":"2026-01-21T14:12:39.346436Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == "cancel_order"sv);
    CHECK(obj.result.cl_ord_id == "6a3260914d000200"sv);
    CHECK(obj.success == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

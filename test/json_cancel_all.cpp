/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::CancelAll;

TEST_CASE("success", "[json_cancel_all]") {
  auto message = R"({)"
                 R"("method":"cancel_all",)"
                 R"("result":{)"
                 R"("count":3)"
                 R"(},)"
                 R"("success":true,)"
                 R"("time_in":"2026-01-21T14:36:04.383462Z",)"
                 R"("time_out":"2026-01-21T14:36:04.385719Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == "cancel_all"sv);
    CHECK(obj.result.count == 3);
    CHECK(obj.success == true);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

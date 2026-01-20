/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Error;

TEST_CASE("snapshot", "[json_error]") {
  auto message = R"({)"
                 R"("error":"Malformed request",)"
                 R"("method":"error",)"
                 R"("success":false,)"
                 R"("time_in":"2026-01-19T15:14:14.834598Z",)"
                 R"("time_out":"2026-01-19T15:14:14.834611Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.error == "Malformed request"sv);
    CHECK(obj.method == "error"sv);
    CHECK(obj.success == false);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

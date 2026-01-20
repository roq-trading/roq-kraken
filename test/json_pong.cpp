/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Pong2;

TEST_CASE("snapshot", "[json_pong]") {
  auto message = R"({)"
                 R"("method":"pong",)"
                 R"("req_id":694480169830934,)"
                 R"("time_in":"2026-01-19T15:20:59.673712Z",)"
                 R"("time_out":"2026-01-19T15:20:59.673717Z")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.method == "pong"sv);
    CHECK(obj.req_id == 694480169830934);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

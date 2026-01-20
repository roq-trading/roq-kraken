/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = json::Executions;

TEST_CASE("empty", "[json_executions]") {
  auto message = R"({)"
                 R"("channel":"executions",)"
                 R"("type":"snapshot",)"
                 R"("data":[],)"
                 R"("sequence":1)"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "executions"sv);
    CHECK(obj.type == json::Type::SNAPSHOT);
    REQUIRE(std::empty(obj.data));
    CHECK(obj.sequence == 1);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

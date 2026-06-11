/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using namespace Catch::literals;

using value_type = protocol::json::Heartbeat;

TEST_CASE("snapshot", "[json_heartbeat]") {
  auto message = R"({)"
                 R"("channel":"heartbeat")"
                 R"(})"sv;
  auto helper = [](value_type const &obj) { CHECK(obj.channel == "heartbeat"sv); };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

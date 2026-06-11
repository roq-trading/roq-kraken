/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "parser_tester.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using value_type = protocol::json::Status;

TEST_CASE("simple", "[json_status]") {
  auto message = R"({)"
                 R"("channel":"status",)"
                 R"("type":"update",)"
                 R"("data":[{)"
                 R"("version":"2.0.10",)"
                 R"("system":"online",)"
                 R"("api_version":"v2",)"
                 R"("connection_id":4502598587506258319)"
                 R"(})"
                 R"(])"
                 R"(})"sv;
  auto helper = [](value_type const &obj) {
    CHECK(obj.channel == "status"sv);
    CHECK(obj.type == "update"sv);
    REQUIRE(std::size(obj.data) == 1);
    auto &d0 = obj.data[0];
    CHECK(d0.version == "2.0.10"sv);
    CHECK(d0.system == "online"sv);
    CHECK(d0.api_version == "v2"sv);
    CHECK(d0.connection_id == 4502598587506258319);
  };
  ParserTester<value_type>::dispatch(helper, message, 8192, 1);
}

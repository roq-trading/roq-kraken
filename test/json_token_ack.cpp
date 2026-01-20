/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/json/token_ack.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using value_type = json::TokenAck;

TEST_CASE("simple", "[json_token_ack]") {
  auto const message = R"({)"
                       R"("error":[],)"
                       R"("result":{)"
                       R"("expires":900,)"
                       R"("token":"QJFpl0/Kd0DaTbZnGE7L2agfYZS2k1aqgB7TD2btgww")"
                       R"(})"
                       R"(})"sv;
  auto helper = [&](value_type &obj) {
    CHECK(std::empty(obj.error));
    CHECK(obj.result.expires == 900);
    CHECK(obj.result.token == "QJFpl0/Kd0DaTbZnGE7L2agfYZS2k1aqgB7TD2btgww"sv);
  };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

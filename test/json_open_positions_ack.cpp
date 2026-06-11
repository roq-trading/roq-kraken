/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/protocol/json/open_positions_ack.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

using value_type = protocol::json::OpenPositionsAck;

TEST_CASE("empty", "[json_open_positions_ack]") {
  auto const message = R"({)"
                       R"("error":[],)"
                       R"("result":{})"
                       R"(})"sv;
  auto helper = [&](value_type &obj) { CHECK(std::empty(obj.error)); };
  core::json::BufferStack buffers{8192, 1};
  value_type obj{message, buffers};
  helper(obj);
}

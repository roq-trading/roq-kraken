/* Copyright (c) 2017-2020,
 Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/kraken/json/result.hpp"
#include "roq/kraken/json/token.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

TEST_CASE("json_token_simple", "[json_token]") {
  auto const message = R"({)"
                       R"("error":[],)"
                       R"("result":{)"
                       R"("expires":900,)"
                       R"("token":"zgJjbFFVjINqeU/vIu+XKGC0Mjh5z/frJdOaQpkTMkk")"
                       R"(})"
                       R"(})"sv;
  bool found = false;
  std::vector<std::byte> buffer(8192);
  json::Result::dispatch<json::Token>(
      message,
      buffer,
      []([[maybe_unused]] std::span<std::string_view> const &errors) { FAIL(); },
      [&](json::Token const &) { found = true; });
  CHECK(found == true);
}

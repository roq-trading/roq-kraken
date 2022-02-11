/* Copyright (c) 2017-2020,
 Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/kraken/json/result.h"
#include "roq/kraken/json/token.h"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

TEST(json_spread, simple) {
  const auto message = R"({)"
                       R"("error":[],)"
                       R"("result":{)"
                       R"("expires":900,)"
                       R"("token":"zgJjbFFVjINqeU/vIu+XKGC0Mjh5z/frJdOaQpkTMkk")"
                       R"(})"
                       R"(})"sv;
  bool found = false;
  core::Buffer buffer_(8192);
  core::json::Buffer buffer(buffer_);
  json::Result::dispatch<json::Token>(
      message,
      buffer,
      [](const std::span<std::string_view> &errors) { FAIL(); },
      [&](const json::Token &token) { found = true; });
  EXPECT_EQ(found, true);
}

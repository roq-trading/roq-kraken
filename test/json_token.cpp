/* Copyright (c) 2017-2020,
 Hans Erik Thrane */

#include <gtest/gtest.h>

#include "roq/kraken/json/result.h"
#include "roq/kraken/json/token.h"

using namespace roq;  // NOLINT
using namespace roq::kraken;  // NOLINT

TEST(json_token, parse) {
  const std::string_view message =
    R"({)"
    R"("error":[],)"
    R"("result":{)"
      R"("token":"abc",)"
      R"("expires":123)"
    R"(})"
    R"(})";
  core::utils::Buffer buffer_(8192);
  core::json::Buffer buffer(buffer_);
  auto token = json::Result::parse<json::Token>(
      message,
      buffer);
  EXPECT_EQ(token.token, "abc");
  EXPECT_EQ(token.expires, 123);
}

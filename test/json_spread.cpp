/* Copyright (c) 2017-2020,
 Hans Erik Thrane */

#include <catch2/catch.hpp>

#include "roq/core/datetime.h"

#include "roq/kraken/json/parser_public.h"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

namespace {
struct Handler : public json::ParserPublic::Handler {
 protected:
  void operator()(const server::Trace<json::Error> &) override { FAIL(); }
  void operator()(const server::Trace<json::SystemStatus> &) override { FAIL(); }
  void operator()(const server::Trace<json::Pong> &) override { FAIL(); }
  void operator()(const server::Trace<json::Heartbeat> &) override { FAIL(); }
  void operator()(const server::Trace<json::SubscriptionStatus> &) override { FAIL(); }

  void operator()(
      const server::Trace<json::Trade> &, [[maybe_unused]] const std::string_view &pair) override {
    FAIL();
  }
  void operator()(
      const server::Trace<json::Spread> &, [[maybe_unused]] const std::string_view &pair) override {
    found = true;
  }
  void operator()(
      const server::Trace<json::Book> &, [[maybe_unused]] const std::string_view &pair) override {
    FAIL();
  }

 private:
  json::Book _book;
  std::string_view _pair;

 public:
  bool found = false;
};
}  // namespace

TEST_CASE("json_spread_simple", "json_spread") {
  const auto message = R"([)"
                       R"(1061,)"
                       R"(["62.203000","62.436000","1644586454.291317","8.60138332","0.18317004"],)"
                       R"("spread",)"
                       R"("MLN/USD")"
                       R"(])"sv;
  Handler handler;
  core::Buffer buffer_(8192);
  core::json::Buffer buffer(buffer_);
  auto trace_info = server::create_trace_info();
  json::ParserPublic::dispatch(handler, message, buffer, trace_info);
  CHECK(handler.found == true);
}

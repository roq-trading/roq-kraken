/* Copyright (c) 2017-2020,
 Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/core/datetime.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/kraken/json/parser_public.hpp"

using namespace roq;
using namespace roq::kraken;

using namespace std::literals;

namespace {
struct Handler : public json::ParserPublic::Handler {
 protected:
  void operator()(Trace<json::Error> const &) override { FAIL(); }
  void operator()(Trace<json::SystemStatus> const &) override { FAIL(); }
  void operator()(Trace<json::Pong> const &) override { FAIL(); }
  void operator()(Trace<json::Heartbeat> const &) override { FAIL(); }
  void operator()(Trace<json::SubscriptionStatus> const &) override { FAIL(); }

  void operator()(Trace<json::Trade> const &, [[maybe_unused]] std::string_view const &pair) override { FAIL(); }
  void operator()(Trace<json::Spread> const &, [[maybe_unused]] std::string_view const &pair) override { found = true; }
  void operator()(Trace<json::Book> const &, [[maybe_unused]] std::string_view const &pair) override { FAIL(); }

 private:
  json::Book _book;
  std::string_view _pair;

 public:
  bool found = false;
};
}  // namespace

TEST_CASE("json_spread_simple", "[json_spread]") {
  auto const message = R"([)"
                       R"(1061,)"
                       R"(["62.203000","62.436000","1644586454.291317","8.60138332","0.18317004"],)"
                       R"("spread",)"
                       R"("MLN/USD")"
                       R"(])"sv;
  Handler handler;
  core::json::BufferStack buffer{8192, 1};
  TraceInfo trace_info;
  json::ParserPublic::dispatch(handler, message, buffer, trace_info);
  CHECK(handler.found == true);
}

/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include "roq/server/flags/settings.hpp"

#include "roq/kraken/flags/flags.hpp"
#include "roq/kraken/flags/misc.hpp"
#include "roq/kraken/flags/rest.hpp"
#include "roq/kraken/flags/ws.hpp"

namespace roq {
namespace kraken {

struct Settings final : public server::flags::Settings, public flags::Flags {
  explicit Settings(args::Parser const &);

  flags::Misc misc;
  flags::REST rest;
  flags::WS ws;
};

}  // namespace kraken
}  // namespace roq

template <>
struct fmt::formatter<roq::kraken::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::kraken::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(misc={}, )"
        R"(rest={}, )"
        R"(ws={}, )"
        R"(server={})"
        R"(}})"sv,
        value.misc,
        value.rest,
        value.ws,
        static_cast<roq::server::Settings const &>(value));
  }
};

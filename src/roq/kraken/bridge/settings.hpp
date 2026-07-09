/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/server/bridge/settings.hpp"

#include "roq/kraken/flags/settings.hpp"

namespace roq {
namespace kraken {
namespace bridge {

struct Settings final : public flags::Settings {
  explicit Settings(args::Parser const &);

  server::bridge::Settings bridge;
};

}  // namespace bridge
}  // namespace kraken
}  // namespace roq

template <>
struct fmt::formatter<roq::kraken::bridge::Settings> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::kraken::bridge::Settings const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(gateway={}, )"
        R"(bridge={})"
        R"(}})"sv,
        static_cast<roq::kraken::flags::Settings const &>(value),
        value.bridge);
  }
};

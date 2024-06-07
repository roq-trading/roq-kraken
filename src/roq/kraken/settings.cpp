/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/kraken/settings.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

Settings::Settings(args::Parser const &args)
    : server::flags::Settings{args, ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER}, flags::Flags{flags::Flags::create()}, misc{flags::Misc::create()},
      rest{flags::REST::create()}, ws{flags::WS::create()} {
  log::info("settings={}"sv, *this);
}

}  // namespace kraken
}  // namespace roq

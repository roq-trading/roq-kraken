/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/application.hpp"

#include "roq/kraken/config.hpp"
#include "roq/kraken/gateway.hpp"
#include "roq/kraken/settings.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === CONSTANTS ===

namespace {
uint8_t const API_2 = {};
}

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  Settings settings{args};
  Config config{settings};
  auto context = server::create_io_context(settings);
  server::Trading<Gateway>{settings, config, *context, API_2}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kraken
}  // namespace roq

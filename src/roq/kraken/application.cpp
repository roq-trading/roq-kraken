/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/kraken/application.hpp"

#include "roq/kraken/config.hpp"
#include "roq/kraken/gateway.hpp"
#include "roq/kraken/settings.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === CONSTANTS ===

namespace {
auto const TYPE = server::Type::ORDER_MANAGEMENT;
}

// === IMPLEMENTATION ===

int Application::main(int, char **) {
  auto settings = Settings::create(TYPE);
  Config config;
  auto context = server::create_io_context();
  server::Trading<Gateway>{settings, config, *context}.dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kraken
}  // namespace roq

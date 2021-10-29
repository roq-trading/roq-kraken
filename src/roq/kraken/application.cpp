/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/application.h"

#include "roq/kraken/config.h"
#include "roq/kraken/flags.h"
#include "roq/kraken/gateway.h"

using namespace std::literals;

namespace roq {
namespace kraken {

int Application::main(int, char **) {
  log::info(R"(Parse config_file="{}")"sv, Flags::config_file());
  Config config(Flags::config_file(), Flags::secrets_file());
  log::info<1>("config={}"sv, config);
  log::info("Starting the gateway"sv);
  roq::server::Trading<Gateway>(ROQ_PACKAGE_NAME, config).dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kraken
}  // namespace roq

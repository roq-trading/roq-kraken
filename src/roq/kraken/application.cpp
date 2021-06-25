/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/application.h"

#include "roq/kraken/config.h"
#include "roq/kraken/flags.h"
#include "roq/kraken/gateway.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

int Application::main(int, char **) {
  log::info(R"(Parse config_file="{}")"_sv, Flags::config_file());
  Config config(Flags::config_file());
  log::info<1>("config={}"_sv, config);
  log::info("Starting the gateway"_sv);
  roq::server::Trading<Gateway>(ROQ_PACKAGE_NAME, config, server::RequestIdType::SEQUENTIAL, config)
      .dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kraken
}  // namespace roq

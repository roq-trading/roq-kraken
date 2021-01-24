/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/application.h"

#include "roq/kraken/config.h"
#include "roq/kraken/flags.h"
#include "roq/kraken/gateway.h"

namespace roq {
namespace kraken {

int Application::main(int, char **) {
  LOG(INFO)("Parse configuration");
  Config config(Flags::config_file());
  VLOG(1)("config={}", config);
  LOG(INFO)("Starting the gateway");
  roq::server::Trading<Gateway>(ROQ_PACKAGE_NAME, config, server::RequestIdType::SEQUENTIAL, config)
      .dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kraken
}  // namespace roq

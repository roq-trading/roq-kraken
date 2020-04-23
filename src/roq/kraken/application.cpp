/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/application.h"

#include "roq/kraken/config.h"
#include "roq/kraken/gateway.h"
#include "roq/kraken/options.h"

namespace roq {
namespace kraken {

int Application::main(int, char **) {
  LOG(INFO)("Parse configuration");
  Config config(FLAGS_config_file);
  VLOG(1)(FMT_STRING("config={}"), config);
  LOG(INFO)("Starting the gateway");
  roq::server::Trading<Gateway>(
      PACKAGE_NAME,
      config,
      FLAGS_listen,
      server::RequestIdType::SEQUENTIAL,
      config).dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kraken
}  // namespace roq

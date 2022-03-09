/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/application.hpp"

#include "roq/kraken/config.hpp"
#include "roq/kraken/flags.hpp"
#include "roq/kraken/gateway.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

int Application::main(int, char **) {
  log::info(R"(Parse config_file="{}")"sv, Flags::config_file());
  Config config(Flags::config_file(), Flags::secrets_file());
  log::info<1>("config={}"sv, config);
  log::info("Starting the gateway"sv);
  roq::server::Trading<Gateway>(ROQ_PACKAGE_NAME, ROQ_BUILD_NUMBER, {}, config).dispatch();
  return EXIT_SUCCESS;
}

}  // namespace kraken
}  // namespace roq

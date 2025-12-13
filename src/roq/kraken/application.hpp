/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/service.hpp"

namespace roq {
namespace kraken {

struct Application final : public roq::Service {
  using roq::Service::Service;

 protected:
  int main(args::Parser const &) override;
};

}  // namespace kraken
}  // namespace roq

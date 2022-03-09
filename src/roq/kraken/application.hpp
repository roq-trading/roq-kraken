/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include "roq/service.hpp"

namespace roq {
namespace kraken {

class Application final : public roq::Service {
 public:
  using roq::Service::Service;

 protected:
  int main(int, char **) override;
};

}  // namespace kraken
}  // namespace roq

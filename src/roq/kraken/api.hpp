/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/kraken/settings.hpp"

namespace roq {
namespace kraken {

struct API final {
  struct {
    std::string_view assets;
    std::string_view asset_pairs;
  } market_data;
  struct {
    std::string_view get_web_sockets_token;
    std::string_view open_positions;
  } order_management;

  // factory
  static API create(Settings const &);
};

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string_view>

namespace roq {
namespace kraken {
namespace gateway {

struct TokenUpdate final {
  std::string_view account;
  std::string_view token;
};

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

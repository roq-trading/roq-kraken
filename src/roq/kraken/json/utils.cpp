/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace json {

// === IMPLEMENTATION ===

roq::Error guess_error([[maybe_unused]] int32_t code) {
  return {};
}

roq::Error guess_error([[maybe_unused]] std::string_view const &error) {
  return {};
}

}  // namespace json
}  // namespace kraken
}  // namespace roq

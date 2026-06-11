/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/protocol/json/utils.hpp"

#include "roq/utils/hash/fnv.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace protocol {
namespace json {

// === IMPLEMENTATION ===

roq::Error guess_error([[maybe_unused]] int32_t code) {
  return {};
}

roq::Error guess_error(std::string_view const &error) {
  auto key = utils::hash::FNV::compute(error);
  switch (key) {
    case utils::hash::FNV::compute("EOrder:Unknown order"sv):
      return Error::TOO_LATE_TO_MODIFY_OR_CANCEL;
  }
  return {};
}

}  // namespace json
}  // namespace protocol
}  // namespace kraken
}  // namespace roq

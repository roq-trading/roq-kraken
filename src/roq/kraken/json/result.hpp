/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <stdexcept>
#include <string_view>

#include "roq/core/json/array.hpp"
#include "roq/core/json/buffer.hpp"
#include "roq/core/json/parser.hpp"

namespace roq {
namespace kraken {
namespace json {

struct Result final {
  template <typename T, typename E, typename H>
  static void dispatch(
      const std::string_view &message,
      core::json::Buffer &buffer,
      E error_handler,
      H result_handler) {
    using namespace std::literals;
    core::json::Parser parser(message);
    auto root = parser.root();
    for (auto [key, value] : std::get<core::json::object_t>(root)) {
      if (key.compare("error"sv) == 0) {
        auto error = core::json::Array<std::span<std::string_view>, core::json::array_t>::parse(
            buffer, std::get<core::json::array_t>(value));
        if (std::size(error) > 0) {
          error_handler(error);
          return;
        }
      } else if (key.compare("result"sv) == 0) {
        T obj(value);  // note! no buffer
        result_handler(obj);
        return;
      } else {
        throw RuntimeError(R"(Unexpected key="{}")"sv, key);
      }
    }
    throw RuntimeError(R"(Didn't find key in {"error", "result"})"sv);
  }
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

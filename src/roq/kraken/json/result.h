/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <stdexcept>
#include <string_view>

#include "roq/core/json/array.h"
#include "roq/core/json/buffer.h"
#include "roq/core/json/parser.h"

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
    core::json::Parser parser(message);
    auto root = parser.root();
    for (auto [key, value] : std::get<core::json::object_t>(root)) {
      if (key.compare("error") == 0) {
        auto error = core::json::Array<roq::span<std::string_view>, core::json::array_t>::parse(
            buffer, std::get<core::json::array_t>(value));
        if (std::size(error) > 0) {
          error_handler(error);
          return;
        }
      } else if (key.compare("result") == 0) {
        T obj(value);  // note! no buffer
        result_handler(obj);
        return;
      } else {
        throw std::runtime_error(fmt::format(R"(Unexpected key="{}")", key));
      }
    }
    throw std::runtime_error(R"(Didn't find key in {"error", "result"})");
  }
};

}  // namespace json
}  // namespace kraken
}  // namespace roq

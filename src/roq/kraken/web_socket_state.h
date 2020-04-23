/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <string_view>

#ifdef VERSION
#undef VERSION
#endif


namespace roq {
namespace kraken {

struct WebSocketState final {
  enum type_t {
    UNDEFINED,
    SYMBOLS,
    TRADING_BALANCE,
    ORDERS,
    DONE
  };

  constexpr WebSocketState() = default;
  constexpr WebSocketState(type_t type) : _type(type) {}  // NOLINT

  WebSocketState& operator++() {
    _type = static_cast<type_t>(static_cast<int>(_type) + 1);
    return *this;
  }

  constexpr operator type_t() const {
    return _type;
  }

  std::string_view as_text() const;
  std::string_view as_raw_text() const;

 private:
  type_t _type = type_t::UNDEFINED;
};

}  // namespace kraken
}  // namespace roq


template <>
struct fmt::formatter<roq::kraken::WebSocketState> {
  template <typename Context>
  constexpr auto parse(Context& context) {
    return context.begin();
  }
  template <typename Context>
  auto format(
      const roq::kraken::WebSocketState& value,
      Context& context) {
    return format_to(
        context.out(),
        "{}",
        value.as_text());
  }
};

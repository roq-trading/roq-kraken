/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <utility>
#include <vector>

#include "roq/api.hpp"
#include "roq/server.hpp"

#include "roq/core/symbols.hpp"

#include "roq/kraken/gateway/api.hpp"
#include "roq/kraken/gateway/settings.hpp"

namespace roq {
namespace kraken {
namespace gateway {

struct Shared final {
  Shared(server::Dispatcher &, Settings const &);

  Shared(Shared const &) = delete;

  std::string_view next_request_id();

  auto discard_symbol(std::string_view const &name) const { return dispatcher.discard_symbol(name); }

  template <typename... Args>
  auto operator()(Args &&...args) {
    return dispatcher(std::forward<Args>(args)...);
  }

 public:
  std::vector<MBPUpdate> bids, asks;
  std::vector<Trade> trades;

  server::Dispatcher &dispatcher;

 public:
  Settings const &settings;
  API const api;

 private:
  uint32_t request_id_ = {};
  std::string request_id_encode_buffer_;

 public:
  core::Symbols symbols;
  utils::unordered_set<std::string> all_symbols;
};

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

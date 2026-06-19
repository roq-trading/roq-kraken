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

  server::Dispatcher &dispatcher;

  Settings const &settings;
  API const api;

  std::vector<MBPUpdate> bids, asks, final_bids, final_asks;
  std::vector<Trade> trades;

  core::Symbols symbols;
  utils::unordered_set<std::string> all_symbols;

 private:
  uint32_t request_id_ = {};
  std::string request_id_encode_buffer_;
};

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/promise.h"

#include "roq/core/buffer.h"

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/client.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/kraken/order_entry_state.h"
#include "roq/kraken/security.h"
#include "roq/kraken/shared.h"

#include "roq/kraken/json/asset_pairs.h"
#include "roq/kraken/json/assets.h"
#include "roq/kraken/json/positions.h"
#include "roq/kraken/json/token.h"

namespace roq {
namespace kraken {

class OrderEntry final : public core::web::Client::Handler {
 public:
  struct TokenUpdate final {
    std::string_view account;
    std::string_view token;
  };

  struct SymbolsUpdate final {
    std::vector<std::string> &symbols;
  };

  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<ReferenceData> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<MarketStatus> &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(TokenUpdate &) = 0;
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  OrderEntry(
      Handler &, core::io::Context &context, uint16_t stream_id, Security &, Shared &, bool master);

  OrderEntry(OrderEntry &&) = delete;
  OrderEntry(const OrderEntry &) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  uint16_t operator()(const Event<CreateOrder> &, const std::string_view &request_id);
  uint16_t operator()(
      const Event<ModifyOrder> &, const std::string_view &request_id, const server::OMS_Order &);
  uint16_t operator()(
      const Event<CancelOrder> &, const std::string_view &request_id, const server::OMS_Order &);
  uint16_t operator()(const Event<CancelAllOrders> &, const std::string_view &request_id);

 protected:
  void operator()(const core::web::Client::Connected &) override;
  void operator()(const core::web::Client::Disconnected &) override;
  void operator()(const core::web::Client::Latency &) override;

  void operator()(ConnectionStatus);

  template <typename T>
  void get(std::function<void(const core::Promise<T> &)> &&callback);

  uint32_t download(OrderEntryState);

  void download_token();
  void download_assets();
  void download_asset_pairs();
  void download_balance();
  void download_open_positions();

  void operator()(const json::Token &);
  void operator()(const json::Assets &);
  void operator()(const json::AssetPairs &);
  void operator()(const json::Positions &);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const bool master_;
  // connection
  core::web::Client connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile assets, asset_pairs, balance, open_positions, get_web_sockets_token;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
  // security
  Security &security_;
  // cache
  Shared &shared_;
  absl::flat_hash_set<std::string> all_symbols_;  // only used by master
  // state
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
  ConnectionStatus status_ = {};
  server::Download<OrderEntryState> download_;
};

}  // namespace kraken
}  // namespace roq

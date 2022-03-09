/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_set.h>

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/buffer.hpp"
#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/core/io/context.hpp"

#include "roq/core/web/client.hpp"

#include "roq/server.hpp"

#include "roq/kraken/order_entry_state.hpp"
#include "roq/kraken/security.hpp"
#include "roq/kraken/shared.hpp"

#include "roq/kraken/json/positions.hpp"
#include "roq/kraken/json/token.hpp"

namespace roq {
namespace kraken {

class OrderEntry final : public core::web::Client::Handler {
 public:
  struct TokenUpdate final {
    std::string_view account;
    std::string_view token;
  };

  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<ReferenceData> &, bool is_last) = 0;
    virtual void operator()(const server::Trace<MarketStatus> &, bool is_last) = 0;
    // cross-communication
    virtual void operator()(TokenUpdate &) = 0;
  };

  OrderEntry(Handler &, core::io::Context &context, uint16_t stream_id, Security &, Shared &);

  OrderEntry(OrderEntry &&) = delete;
  OrderEntry(const OrderEntry &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  uint16_t operator()(
      const Event<CreateOrder> &, const oms::Order &, const std::string_view &request_id);
  uint16_t operator()(
      const Event<ModifyOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id);
  uint16_t operator()(
      const Event<CancelOrder> &,
      const oms::Order &,
      const std::string_view &request_id,
      const std::string_view &previous_request_id);

  uint16_t operator()(const Event<CancelAllOrders> &, const std::string_view &request_id);

 protected:
  void operator()(const core::web::Client::Connected &) override;
  void operator()(const core::web::Client::Disconnected &) override;
  void operator()(const core::web::Client::Latency &) override;

  void operator()(ConnectionStatus);

  uint32_t download(OrderEntryState);

  void get_token();
  void get_token_ack(const server::Trace<core::web::Response> &, uint32_t sequence);
  void operator()(const server::Trace<json::Token> &);

  void get_positions();
  void get_positions_ack(const server::Trace<core::web::Response> &, uint32_t sequence);
  void operator()(const server::Trace<json::Positions> &);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  // connection
  core::web::Client connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile get_web_sockets_token, get_web_sockets_token_ack, positions,
        positions_ack;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
  // security
  Security &security_;
  // cache
  Shared &shared_;
  // state
  bool ready_ = false;
  ConnectionStatus status_ = {};
  core::Download<OrderEntryState> download_;
};

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/client_socket.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/kraken/shared.h"

#include "roq/kraken/json/parser_public.h"

namespace roq {
namespace kraken {

class MarketData final : public core::web::ClientSocket::Handler,
                         public json::ParserPublic::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
    virtual void operator()(const server::Trace<TopOfBook> &, bool is_last) = 0;
    virtual void operator()(
        const server::Trace<MarketByPriceUpdate> &, bool is_last, bool refresh) = 0;
    virtual void operator()(const server::Trace<TradeSummary> &, bool is_last) = 0;
  };

  MarketData(Handler &, core::io::Context &, uint16_t stream_id, Shared &, size_t index);

  MarketData(MarketData &&) = delete;
  MarketData(const MarketData &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void subscribe(size_t start_from = 0);

 protected:
  void operator()(const core::web::ClientSocket::Connected &) override;
  void operator()(const core::web::ClientSocket::Disconnected &) override;
  void operator()(const core::web::ClientSocket::Ready &) override;
  void operator()(const core::web::ClientSocket::Close &) override;
  void operator()(const core::web::ClientSocket::Latency &) override;
  void operator()(const core::web::ClientSocket::Text &) override;
  void operator()(const core::web::ClientSocket::Binary &) override;

  void operator()(ConnectionStatus);

  void subscribe(const roq::span<std::string const> &symbols);

  void subscribe(const std::string_view &name, const roq::span<std::string const> &symbols);

  void subscribe_book(const std::string_view &symbol);
  void unsubscribe_book(const std::string_view &symbol);

  // json::ParserPublic::Handler

  void operator()(const server::Trace<json::Error> &) override;
  void operator()(const server::Trace<json::SystemStatus> &) override;
  void operator()(const server::Trace<json::Pong> &) override;
  void operator()(const server::Trace<json::Heartbeat> &) override;
  void operator()(const server::Trace<json::SubscriptionStatus> &) override;

  void operator()(const server::Trace<json::Trade> &, const std::string_view &pair) override;
  void operator()(const server::Trace<json::Spread> &, const std::string_view &pair) override;
  void operator()(const server::Trace<json::Book> &, const std::string_view &pair) override;

 private:
  void parse(const std::string_view &message);

  void reset();

  void resubscribe(const server::TraceInfo &, const std::string_view &symbol);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const size_t index_;
  // web socket
  core::web::ClientSocket connection_;
  // buffers
  core::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile parse;
  } profile_;
  struct {
    core::metrics::Latency ping, heartbeat;
  } latency_;
  // cache
  Shared &shared_;
  // state
  std::chrono::nanoseconds next_heartbeat_ = {};
  ConnectionStatus status_ = {};
  // experimental
  absl::flat_hash_set<std::string> latch_;
};

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2022, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/download.hpp"

#include "roq/core/metrics/counter.hpp"
#include "roq/core/metrics/latency.hpp"
#include "roq/core/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/server.hpp"

#include "roq/kraken/shared.hpp"

#include "roq/kraken/json/parser_public.hpp"

namespace roq {
namespace kraken {

class MarketData final : public web::socket::Client::Handler, public json::ParserPublic::Handler {
 public:
  struct Handler {
    virtual void operator()(Trace<StreamStatus const> const &) = 0;
    virtual void operator()(Trace<ExternalLatency const> const &) = 0;
    virtual void operator()(Trace<TopOfBook const> const &, bool is_last) = 0;
    virtual void operator()(Trace<MarketByPriceUpdate const> const &, bool is_last, bool refresh) = 0;
    virtual void operator()(Trace<TradeSummary const> const &, bool is_last) = 0;
  };

  MarketData(Handler &, io::Context &, uint16_t stream_id, Shared &, size_t index);

  MarketData(MarketData &&) = delete;
  MarketData(MarketData const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

  void subscribe(size_t start_from = 0);

 protected:
  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

  void operator()(ConnectionStatus);

  void subscribe(std::span<Symbol const> const &symbols);

  void subscribe(std::string_view const &name, std::span<Symbol const> const &symbols);

  void subscribe_book(std::string_view const &symbol);
  void unsubscribe_book(std::string_view const &symbol);

  // json::ParserPublic::Handler

  void operator()(Trace<json::Error const> const &) override;
  void operator()(Trace<json::SystemStatus const> const &) override;
  void operator()(Trace<json::Pong const> const &) override;
  void operator()(Trace<json::Heartbeat const> const &) override;
  void operator()(Trace<json::SubscriptionStatus const> const &) override;

  void operator()(Trace<json::Trade const> const &, std::string_view const &pair) override;
  void operator()(Trace<json::Spread const> const &, std::string_view const &pair) override;
  void operator()(Trace<json::Book const> const &, std::string_view const &pair) override;

 private:
  void parse(std::string_view const &message);

  void reset();

  void resubscribe(TraceInfo const &, std::string_view const &symbol);

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const size_t index_;
  // web socket
  std::unique_ptr<web::socket::Client> connection_;
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
  absl::flat_hash_set<Symbol> latch_;
};

}  // namespace kraken
}  // namespace roq

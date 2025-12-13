/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/download.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/kraken/shared.hpp"

#include "roq/kraken/json/parser_public.hpp"

namespace roq {
namespace kraken {

struct MarketData final : public web::socket::Client::Handler, public json::ParserPublic::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    virtual void operator()(Trace<TopOfBook> const &, bool is_last) = 0;
    virtual void operator()(Trace<MarketByPriceUpdate> const &, bool is_last) = 0;
    virtual void operator()(Trace<TradeSummary> const &, bool is_last) = 0;
  };

  MarketData(Handler &, io::Context &, uint16_t stream_id, Shared &, size_t index);

  MarketData(MarketData const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

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

  void operator()(Trace<json::Error> const &) override;
  void operator()(Trace<json::SystemStatus> const &) override;
  void operator()(Trace<json::Pong> const &) override;
  void operator()(Trace<json::Heartbeat> const &) override;
  void operator()(Trace<json::SubscriptionStatus> const &) override;

  void operator()(Trace<json::Trade> const &, std::string_view const &pair) override;
  void operator()(Trace<json::Spread> const &, std::string_view const &pair) override;
  void operator()(Trace<json::Book> const &, std::string_view const &pair) override;

 private:
  void parse(std::string_view const &message);

  void reset();

  void resubscribe(TraceInfo const &, std::string_view const &symbol);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  size_t const index_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile parse;
  } profile_;
  struct {
    utils::metrics::Latency ping, heartbeat;
  } latency_;
  // cache
  Shared &shared_;
  // state
  std::chrono::nanoseconds next_heartbeat_ = {};
  ConnectionStatus status_ = {};
  // experimental
  utils::unordered_set<std::string> latch_;
};

}  // namespace kraken
}  // namespace roq

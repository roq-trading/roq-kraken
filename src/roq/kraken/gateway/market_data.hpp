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

#include "roq/kraken/gateway/shared.hpp"

#include "roq/kraken/protocol/json/parser.hpp"

namespace roq {
namespace kraken {
namespace gateway {

struct MarketData final : public web::socket::Client::Handler, public protocol::json::Parser::Handler {
  struct SymbolsUpdate final {
    std::span<Symbol const> symbols;
  };

  struct Handler {
    virtual void operator()(SymbolsUpdate &) = 0;
  };

  MarketData(Handler &, io::Context &, uint16_t stream_id, Shared &, size_t index);

  MarketData(MarketData const &) = delete;

  bool ready() const { return connection_status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  void subscribe(size_t start_from = 0);

 protected:
  // web::socket::Client::Handler

  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  void subscribe_static();

  void subscribe(std::span<Symbol const> const &symbols);

  void subscribe(std::string_view const &channel);
  void subscribe(std::string_view const &channel, std::span<Symbol const> const &symbols);

  void parse(std::string_view const &message);

  // protocol::json::Parser::Handler

  void operator()(Trace<protocol::json::Status> const &) override;
  void operator()(Trace<protocol::json::Heartbeat> const &) override;

  void operator()(Trace<protocol::json::Error> const &) override;
  void operator()(Trace<protocol::json::Pong> const &) override;
  void operator()(Trace<protocol::json::Subscribe> const &) override;

  void operator()(Trace<protocol::json::Instrument> const &) override;

  void operator()(Trace<protocol::json::Ticker> const &) override;
  void operator()(Trace<protocol::json::Trade> const &) override;
  void operator()(Trace<protocol::json::Book> const &) override;

  void operator()(Trace<protocol::json::Balances> const &) override;
  void operator()(Trace<protocol::json::Executions> const &) override;

  void operator()(Trace<protocol::json::AddOrder> const &) override;
  void operator()(Trace<protocol::json::AmendOrder> const &) override;
  void operator()(Trace<protocol::json::CancelOrder> const &) override;
  void operator()(Trace<protocol::json::CancelAll> const &) override;

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
  ConnectionStatus connection_status_ = {};
};

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

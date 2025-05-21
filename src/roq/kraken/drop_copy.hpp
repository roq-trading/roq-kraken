/* Copyright (c) 2017-2025, Hans Erik Thrane */

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

#include "roq/server.hpp"

#include "roq/kraken/account.hpp"
#include "roq/kraken/drop_copy_state.hpp"
#include "roq/kraken/shared.hpp"

#include "roq/kraken/json/parser_private.hpp"

namespace roq {
namespace kraken {

struct DropCopy final : public web::socket::Client::Handler, public json::ParserPrivate::Handler {
  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
  };

  DropCopy(Handler &, io::Context &, uint16_t stream_id, Account &, Shared &, std::string_view const &token);

  DropCopy(DropCopy const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  void subscribe(std::string_view const &name, std::string_view const &token);

 protected:
  void operator()(web::socket::Client::Connected const &) override;
  void operator()(web::socket::Client::Disconnected const &) override;
  void operator()(web::socket::Client::Ready const &) override;
  void operator()(web::socket::Client::Close const &) override;
  void operator()(web::socket::Client::Latency const &) override;
  void operator()(web::socket::Client::Text const &) override;
  void operator()(web::socket::Client::Binary const &) override;

  void operator()(ConnectionStatus);

  uint32_t download(DropCopyState);

  void subscribe();
  void subscribe(std::string_view const &name);

  void parse(std::string_view const &message);

  void operator()(Trace<json::Error> const &) override;
  void operator()(Trace<json::SystemStatus> const &) override;
  void operator()(Trace<json::Pong> const &) override;
  void operator()(Trace<json::Heartbeat> const &) override;
  void operator()(Trace<json::SubscriptionStatus> const &) override;

  void operator()(Trace<json::AddOrderStatus> const &) override;
  void operator()(Trace<json::CancelOrderStatus> const &) override;

  void operator()(Trace<json::OpenOrders> const &) override;
  void operator()(Trace<json::OwnTrades> const &) override;

  void reset();

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  std::string const token_;
  // web socket
  std::unique_ptr<web::socket::Client> const connection_;
  // buffers
  std::vector<std::byte> decode_buffer_;
  // core::stack::Buffer<char, 32> stack_buffer_;
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
  // account
  Account &account_;
  // cache
  Shared &shared_;
  // state
  bool ready_ = false;
  std::chrono::nanoseconds next_heartbeat_ = {};
  ConnectionStatus status_ = {};
  core::Download<DropCopyState> download_;
};

}  // namespace kraken
}  // namespace roq

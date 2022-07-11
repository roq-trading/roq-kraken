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

#include "roq/core/web/client_socket.hpp"

#include "roq/server.hpp"

#include "roq/kraken/drop_copy_state.hpp"
#include "roq/kraken/security.hpp"
#include "roq/kraken/shared.hpp"

#include "roq/kraken/json/parser_private.hpp"

namespace roq {
namespace kraken {

class DropCopy final : public core::web::ClientSocket::Handler, public json::ParserPrivate::Handler {
 public:
  struct Handler {
    virtual void operator()(Trace<StreamStatus const> const &) = 0;
    virtual void operator()(Trace<ExternalLatency const> const &) = 0;
  };

  DropCopy(Handler &, io::Context &, uint16_t stream_id, Security &, Shared &, std::string_view const &token);

  DropCopy(DropCopy &&) = delete;
  DropCopy(DropCopy const &) = delete;

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

  void subscribe(std::string_view const &name, std::string_view const &token);

 protected:
  void operator()(core::web::ClientSocket::Connected const &) override;
  void operator()(core::web::ClientSocket::Disconnected const &) override;
  void operator()(core::web::ClientSocket::Ready const &) override;
  void operator()(core::web::ClientSocket::Close const &) override;
  void operator()(core::web::ClientSocket::Latency const &) override;
  void operator()(core::web::ClientSocket::Text const &) override;
  void operator()(core::web::ClientSocket::Binary const &) override;

  void operator()(ConnectionStatus);

  uint32_t download(DropCopyState);

  void subscribe();
  void subscribe(std::string_view const &name);

  void parse(std::string_view const &message);

  void operator()(Trace<json::Error const> const &) override;
  void operator()(Trace<json::SystemStatus const> const &) override;
  void operator()(Trace<json::Pong const> const &) override;
  void operator()(Trace<json::Heartbeat const> const &) override;
  void operator()(Trace<json::SubscriptionStatus const> const &) override;

  void operator()(Trace<json::AddOrderStatus const> const &) override;
  void operator()(Trace<json::CancelOrderStatus const> const &) override;

  void operator()(Trace<json::OpenOrders const> const &) override;
  void operator()(Trace<json::OwnTrades const> const &) override;

  void reset();

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const std::string token_;
  // web socket
  core::web::ClientSocket connection_;
  // buffers
  core::Buffer decode_buffer_;
  // core::stack::Buffer<char, 32> stack_buffer_;
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
  // security
  Security &security_;
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

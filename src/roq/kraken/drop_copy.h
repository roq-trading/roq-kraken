/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/socket.h"

#include "roq/download.h"
#include "roq/server.h"

#include "roq/kraken/drop_copy_state.h"
#include "roq/kraken/security.h"
#include "roq/kraken/shared.h"

#include "roq/kraken/json/parser_private.h"

namespace roq {
namespace kraken {

class DropCopy final : public core::web::Socket::Handler, public json::ParserPrivate::Handler {
 public:
  struct Handler {
    virtual void operator()(const server::Trace<StreamStatus> &) = 0;
    virtual void operator()(const server::Trace<ExternalLatency> &) = 0;
  };

  DropCopy(
      Handler &,
      core::io::Context &,
      uint16_t stream_id,
      Security &,
      Shared &,
      const std::string_view &token);

  DropCopy(DropCopy &&) = delete;
  DropCopy(const DropCopy &) = delete;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &);

  void subscribe(const std::string_view &name, const std::string_view &token);

 protected:
  void operator()(const core::web::Socket::Connected &) override;
  void operator()(const core::web::Socket::Disconnected &) override;
  void operator()(const core::web::Socket::Ready &) override;
  void operator()(const core::web::Socket::Close &) override;
  void operator()(const core::web::Socket::Latency &) override;
  void operator()(const core::web::Socket::Text &) override;

  void operator()(ConnectionStatus);

  uint32_t download(DropCopyState);

  void subscribe();
  void subscribe(const std::string_view &name);

  void parse(const std::string_view &message);

  void operator()(const server::Trace<json::Error> &) override;
  void operator()(const server::Trace<json::SystemStatus> &) override;
  void operator()(const server::Trace<json::Pong> &) override;
  void operator()(const server::Trace<json::Heartbeat> &) override;
  void operator()(const server::Trace<json::SubscriptionStatus> &) override;

  void operator()(const server::Trace<json::AddOrderStatus> &) override;
  void operator()(const server::Trace<json::CancelOrderStatus> &) override;

  void operator()(const server::Trace<json::OpenOrders> &) override;
  void operator()(const server::Trace<json::OwnTrades> &) override;

  void reset();

 private:
  Handler &handler_;
  // config
  const uint16_t stream_id_;
  const std::string name_;
  const std::string token_;
  // web socket
  core::web::Socket connection_;
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
  server::Download<DropCopyState> download_;
};

}  // namespace kraken
}  // namespace roq

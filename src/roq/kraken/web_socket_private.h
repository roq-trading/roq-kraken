/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/core/ssl/ssl.h"

#include "roq/core/web/socket.h"

#include "roq/server.h"

#include "roq/kraken/config.h"
#include "roq/kraken/random.h"

#include "roq/kraken/json/parser_private.h"

namespace roq {
namespace kraken {

class WebSocketPrivate final
    : public core::web::Socket::Handler,
      public json::ParserPrivate::Handler {
 public:
  struct Handler {
    virtual void operator()(const WebSocketPrivate&) = 0;
    virtual void operator()(const json::AddOrderStatus&) = 0;
    virtual void operator()(const json::CancelOrderStatus&) = 0;
    virtual void operator()(const json::OpenOrders&) = 0;
    virtual void operator()(const json::OwnTrades&) = 0;
  };

  WebSocketPrivate(
      Handler& handler,
      const Config& config,
      Random& random,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      core::ssl::Context& ssl_context);

  WebSocketPrivate(WebSocketPrivate&&) = delete;
  WebSocketPrivate(const WebSocketPrivate&) = delete;

  bool ready() const;

  void close();

  void operator()(const server::StartEvent&);
  void operator()(const server::StopEvent&);
  void operator()(const server::TimerEvent&);

  void operator()(metrics::Writer& writer);

  void subscribe(
      const std::string_view& name,
      const std::string_view& token);

 protected:
  // core::web::Socket::Handler

  void operator()(const core::web::Socket::Connected&) override;
  void operator()(const core::web::Socket::Disconnected&) override;
  void operator()(const core::web::Socket::Ready&) override;
  void operator()(const core::web::Socket::Close&) override;
  void operator()(const core::web::Socket::Latency&) override;
  void operator()(const core::web::Socket::Text&) override;

  // json::ParserPrivate::Handler

  void operator()(const json::Error&) override;
  void operator()(const json::SystemStatus&) override;
  void operator()(const json::Pong&) override;
  void operator()(const json::Heartbeat&) override;
  void operator()(const json::SubscriptionStatus&) override;

  void operator()(const json::AddOrderStatus&) override;
  void operator()(const json::CancelOrderStatus&) override;

  void operator()(const json::OpenOrders&) override;
  void operator()(const json::OwnTrades&) override;

 protected:
  void reset();

  void parse(const std::string_view& message);

 private:
  Handler& _handler;
  // config
  const std::string _access_key;
  // authentication
  Random& _random;
  // web socket
  core::web::Socket _connection;
  // buffers
  core::utils::Buffer _decode_buffer;
  core::stack::Buffer<char, 32> _stack_buffer;
  // metrics
  struct {
    core::metrics::Counter
      disconnect;
  } _counter;
  struct {
    core::metrics::Profile
      parse;
  } _profile;
  struct {
    core::metrics::Latency
      ping,
      heartbeat;
  } _latency;
};

}  // namespace kraken
}  // namespace roq

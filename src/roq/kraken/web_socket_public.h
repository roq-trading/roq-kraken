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

#include "roq/kraken/json/parser_public.h"

namespace roq {
namespace kraken {

class WebSocketPublic final
    : public core::web::Socket::Handler,
      public json::ParserPublic::Handler {
 public:
  struct Handler {
    virtual void operator()(const WebSocketPublic&) = 0;
    virtual void operator()(
        const json::Trade& trade,
        const std::string_view& pair) = 0;
    virtual void operator()(
        const json::Spread& spread,
        const std::string_view& pair) = 0;
    virtual void operator()(
        const json::Book& book,
        const std::string_view& pair) = 0;
  };

  WebSocketPublic(
      Handler& handler,
      const Config& config,
      Random& random,
      core::event::Base& base,
      core::event::DNSBase& dns_base,
      core::ssl::Context& ssl_context);

  WebSocketPublic(WebSocketPublic&&) = delete;
  WebSocketPublic(const WebSocketPublic&) = delete;

  bool ready() const;

  void close();

  void operator()(const server::StartEvent&);
  void operator()(const server::StopEvent&);
  void operator()(const server::TimerEvent&);

  void operator()(Metrics& metrics);

  template <typename T>
  void subscribe(
      const std::string_view& name,
      const roq::span<T>& pairs);

 protected:
  // core::web::Socket::Handler

  void operator()(const core::web::Socket::Connected&) override;
  void operator()(const core::web::Socket::Disconnected&) override;
  void operator()(const core::web::Socket::Ready&) override;
  void operator()(const core::web::Socket::Close&) override;
  void operator()(const core::web::Socket::Latency&) override;
  void operator()(const core::web::Socket::Text&) override;

  // json::ParserPublic::Handler

  void operator()(const json::Error&) override;
  void operator()(const json::SystemStatus&) override;
  void operator()(const json::Pong&) override;
  void operator()(const json::Heartbeat&) override;
  void operator()(const json::SubscriptionStatus&) override;

  void operator()(
      const json::Trade& trade,
      const std::string_view& pair) override;
  void operator()(
      const json::Spread& spread,
      const std::string_view& pair) override;
  void operator()(
      const json::Book& book,
      const std::string_view& pair) override;

 private:
  void parse(const std::string_view& message);

  void reset();

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

/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/web_socket_private.h"

#include <fmt/format.h>

#include "roq/builtins.h"

#include "roq/core/clock.h"

#include "roq/kraken/options.h"

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "ws_private";

static auto create_counter(
    const std::string_view& function) {
  return core::metrics::Counter(
      FLAGS_name,
      CONNECTION,
      function);
}

static auto create_profile(
    const std::string_view& function) {
  return core::metrics::Profile(
      FLAGS_name,
      CONNECTION,
      function);
}

static auto create_latency(
    const std::string_view& function) {
  return core::metrics::Latency(
      FLAGS_name,
      CONNECTION,
      function);
}
}  // namespace

WebSocketPrivate::WebSocketPrivate(
    Handler& handler,
    const Config& config,
    Random& random,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    core::ssl::Context& ssl_context)
    : _handler(handler),
      _access_key(config.get_access_key()),
      _random(random),
      _connection(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(FLAGS_ws_private_uri),
          std::chrono::seconds { FLAGS_ping_freq_secs },
          FLAGS_decode_buffer_size,  // XXX need read buffer size
          FLAGS_encode_buffer_size,
          []() { return std::string(); }),
      _decode_buffer(FLAGS_decode_buffer_size),
      _counter {
        .disconnect = create_counter("disconnect"),
      },
      _profile {
        .parse = create_profile("parse"),
      },
      _latency {
        .ping = create_latency("ping"),
        .heartbeat = create_latency("heartbeat"),
      } {
}

bool WebSocketPrivate::ready() const {
  return _connection.ready();
}

void WebSocketPrivate::close() {
  _connection.close();
}

void WebSocketPrivate::operator()(const StartEvent&) {
  _connection.start();
}

void WebSocketPrivate::operator()(const StopEvent&) {
  _connection.stop();
}

void WebSocketPrivate::operator()(const TimerEvent& event) {
  _connection.refresh(event.now);
}

void WebSocketPrivate::operator()(Metrics& metrics) {
  metrics
    // counter
    .write(_counter.disconnect)
    // profile
    .write(_profile.parse)
    // latency
    .write(_latency.ping)
    .write(_latency.heartbeat);
}

void WebSocketPrivate::subscribe(
    const std::string_view& name,
    const std::string_view& token) {
  LOG(INFO)(
      FMT_STRING(R"(subscribe name="{}", token="{}")"),
      name,
      token);
  auto message = fmt::format(
      FMT_STRING(
        R"({{)"
        R"("event":"subscribe",)"
        R"("subscription":{{)"
        R"("name":"{}",)"
        R"("token":"{}")"
        R"(}})"
        R"(}})"),
        name,
        token);
  VLOG(3)(
      FMT_STRING(R"(request="{}")"),
      message);
  _connection.send_text(message);
}

void WebSocketPrivate::operator()(const core::web::Socket::Connected&) {
  // note! wait for upgrade
}

void WebSocketPrivate::operator()(const core::web::Socket::Disconnected&) {
  ++_counter.disconnect;
  _handler(*this);
}

void WebSocketPrivate::operator()(const core::web::Socket::Ready&) {
  LOG(INFO)("Ready");
  _handler(*this);
}

void WebSocketPrivate::operator()(const core::web::Socket::Close&) {
}

void WebSocketPrivate::operator()(const core::web::Socket::Latency& latency) {
  _latency.ping.update(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          latency.sample).count());
}

void WebSocketPrivate::operator()(const core::web::Socket::Text& text) {
  parse(text.payload);
}

void WebSocketPrivate::parse(const std::string_view& message) {
  _profile.parse(
      [&]() {
        core::json::Buffer buffer(_decode_buffer);
        auto result = json::ParserPrivate::dispatch(
            *this,
            message,
            buffer);
      });
}

void WebSocketPrivate::operator()(const json::Error& error) {
  LOG(FATAL)(
      FMT_STRING("error={}"),
      error);
}

void WebSocketPrivate::operator()(const json::SystemStatus& system_status) {
  LOG(INFO)(
      FMT_STRING("system_status={}"),
      system_status);
}

void WebSocketPrivate::operator()(const json::Pong& pong) {
  VLOG(1)(
      FMT_STRING("pong={}"),
      pong);
}

void WebSocketPrivate::operator()(const json::Heartbeat& heartbeat) {
  VLOG(1)(
      FMT_STRING("heartbeat={}"),
      heartbeat);
}

void WebSocketPrivate::operator()(
    const json::SubscriptionStatus& subscription_status) {
  LOG(INFO)(
      FMT_STRING("subscription_status={}"),
      subscription_status);
}

void WebSocketPrivate::operator()(
    const json::AddOrderStatus& add_order_status) {
  LOG(INFO)(
      FMT_STRING("add_order_status={}"),
      add_order_status);
  _handler(add_order_status);
}

void WebSocketPrivate::operator()(
    const json::CancelOrderStatus& cancel_order_status) {
  LOG(INFO)(
      FMT_STRING("cancel_order_status={}"),
      cancel_order_status);
  _handler(cancel_order_status);
}

void WebSocketPrivate::operator()(const json::OpenOrders& open_orders) {
  _handler(open_orders);
}

void WebSocketPrivate::operator()(const json::OwnTrades& own_trades) {
  _handler(own_trades);
}

}  // namespace kraken
}  // namespace roq

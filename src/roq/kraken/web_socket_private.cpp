/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/web_socket_private.h"

#include <fmt/format.h>

#include "roq/core/clock.h"

#include "roq/kraken/options.h"

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "ws_private";

static auto create_counter(const std::string_view &function) {
  return core::metrics::Counter(FLAGS_name, CONNECTION, function);
}

static auto create_profile(const std::string_view &function) {
  return core::metrics::Profile(FLAGS_name, CONNECTION, function);
}

static auto create_latency(const std::string_view &function) {
  return core::metrics::Latency(FLAGS_name, CONNECTION, function);
}
}  // namespace

WebSocketPrivate::WebSocketPrivate(
    Handler &handler,
    const Config &config,
    Random &random,
    core::event::Base &base,
    core::event::DNSBase &dns_base,
    core::ssl::Context &ssl_context)
    : _handler(handler), _access_key(config.get_access_key()), _random(random),
      _connection(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(FLAGS_ws_private_uri),
          std::string_view(),  // query
          std::chrono::seconds{FLAGS_ws_private_ping_freq_secs},
          FLAGS_decode_buffer_size,  // XXX need read buffer size
          FLAGS_encode_buffer_size,
          []() { return std::string(); }),
      _decode_buffer(FLAGS_decode_buffer_size),
      _counter{
          .disconnect = create_counter("disconnect"),
      },
      _profile{
          .parse = create_profile("parse"),
      },
      _latency{
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

void WebSocketPrivate::operator()(const Event<Start> &) {
  _connection.start();
}

void WebSocketPrivate::operator()(const Event<Stop> &) {
  _connection.stop();
}

void WebSocketPrivate::operator()(const Event<Timer> &event) {
  _connection.refresh(event.value.now);
}

void WebSocketPrivate::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(_counter.disconnect, metrics::COUNTER)
      // profile
      .write(_profile.parse, metrics::PROFILE)
      // latency
      .write(_latency.ping, metrics::LATENCY)
      .write(_latency.heartbeat, metrics::LATENCY);
}

void WebSocketPrivate::subscribe(
    const std::string_view &name, const std::string_view &token) {
  LOG(INFO)(R"(subscribe name="{}", token="{}")", name, token);
  auto message = fmt::format(
      R"({{)"
      R"("event":"subscribe",)"
      R"("subscription":{{)"
      R"("name":"{}",)"
      R"("token":"{}")"
      R"(}})"
      R"(}})",
      name,
      token);
  VLOG(3)(R"(request="{}")", message);
  _connection.send_text(message);
}

void WebSocketPrivate::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void WebSocketPrivate::operator()(const core::web::Socket::Disconnected &) {
  ++_counter.disconnect;
  _handler(*this);
}

void WebSocketPrivate::operator()(const core::web::Socket::Ready &) {
  LOG(INFO)("Ready");
  _handler(*this);
}

void WebSocketPrivate::operator()(const core::web::Socket::Close &) {
}

void WebSocketPrivate::operator()(const core::web::Socket::Latency &latency) {
  _latency.ping.update(
      std::chrono::duration_cast<std::chrono::nanoseconds>(latency.sample)
          .count());
}

void WebSocketPrivate::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void WebSocketPrivate::parse(const std::string_view &message) {
  _profile.parse([&]() {
    server::TraceInfo trace_info;
    core::json::Buffer buffer(_decode_buffer);
    auto result =
        json::ParserPrivate::dispatch(*this, message, buffer, trace_info);
  });
}

void WebSocketPrivate::operator()(
    const json::Error &error, const server::TraceInfo &) {
  LOG(FATAL)("error={}", error);
}

void WebSocketPrivate::operator()(
    const json::SystemStatus &system_status, const server::TraceInfo &) {
  LOG(INFO)("system_status={}", system_status);
}

void WebSocketPrivate::operator()(
    const json::Pong &pong, const server::TraceInfo &) {
  VLOG(1)("pong={}", pong);
}

void WebSocketPrivate::operator()(
    const json::Heartbeat &heartbeat, const server::TraceInfo &) {
  VLOG(1)("heartbeat={}", heartbeat);
}

void WebSocketPrivate::operator()(
    const json::SubscriptionStatus &subscription_status,
    const server::TraceInfo &) {
  LOG(INFO)("subscription_status={}", subscription_status);
}

void WebSocketPrivate::operator()(
    const json::AddOrderStatus &add_order_status,
    const server::TraceInfo &trace_info) {
  LOG(INFO)("add_order_status={}", add_order_status);
  _handler(add_order_status, trace_info);
}

void WebSocketPrivate::operator()(
    const json::CancelOrderStatus &cancel_order_status,
    const server::TraceInfo &trace_info) {
  LOG(INFO)("cancel_order_status={}", cancel_order_status);
  _handler(cancel_order_status, trace_info);
}

void WebSocketPrivate::operator()(
    const json::OpenOrders &open_orders, const server::TraceInfo &trace_info) {
  _handler(open_orders, trace_info);
}

void WebSocketPrivate::operator()(
    const json::OwnTrades &own_trades, const server::TraceInfo &trace_info) {
  _handler(own_trades, trace_info);
}

}  // namespace kraken
}  // namespace roq

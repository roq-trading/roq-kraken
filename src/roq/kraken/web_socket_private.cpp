/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/web_socket_private.h"

#include <fmt/format.h>

#include "roq/core/clock.h"

#include "roq/kraken/flags.h"

using namespace std::literals;  // NOLINT

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "ws_private"sv;

static auto create_counter(const std::string_view &function) {
  return core::metrics::Counter(Flags::name(), CONNECTION, function);
}

static auto create_profile(const std::string_view &function) {
  return core::metrics::Profile(Flags::name(), CONNECTION, function);
}

static auto create_latency(const std::string_view &function) {
  return core::metrics::Latency(Flags::name(), CONNECTION, function);
}
}  // namespace

WebSocketPrivate::WebSocketPrivate(
    Handler &handler,
    const Config &config,
    Random &random,
    core::event::Base &base,
    core::event::DNSBase &dns_base,
    core::ssl::Context &ssl_context)
    : handler_(handler), access_key_(config.get_access_key()), random_(random),
      connection_(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(Flags::ws_private_uri()),
          std::string_view(),  // query
          std::chrono::seconds{Flags::ws_private_ping_freq_secs()},
          Flags::decode_buffer_size(),  // XXX need read buffer size
          Flags::encode_buffer_size(),
          []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_counter("disconnect"sv),
      },
      profile_{
          .parse = create_profile("parse"sv),
      },
      latency_{
          .ping = create_latency("ping"sv),
          .heartbeat = create_latency("heartbeat"sv),
      } {
}

bool WebSocketPrivate::ready() const {
  return connection_.ready();
}

void WebSocketPrivate::close() {
  connection_.close();
}

void WebSocketPrivate::operator()(const Event<Start> &) {
  connection_.start();
}

void WebSocketPrivate::operator()(const Event<Stop> &) {
  connection_.stop();
}

void WebSocketPrivate::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void WebSocketPrivate::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void WebSocketPrivate::subscribe(const std::string_view &name, const std::string_view &token) {
  LOG(INFO)(R"(subscribe name="{}", token="{}")"sv, name, token);
  auto message = fmt::format(
      R"({{)"
      R"("event":"subscribe",)"
      R"("subscription":{{)"
      R"("name":"{}",)"
      R"("token":"{}")"
      R"(}})"
      R"(}})"sv,
      name,
      token);
  VLOG(3)(R"(request="{}")"sv, message);
  connection_.send_text(message);
}

void WebSocketPrivate::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void WebSocketPrivate::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  handler_(*this);
}

void WebSocketPrivate::operator()(const core::web::Socket::Ready &) {
  LOG(INFO)("Ready"sv);
  handler_(*this);
}

void WebSocketPrivate::operator()(const core::web::Socket::Close &) {
}

void WebSocketPrivate::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .name = CONNECTION,
      .latency = latency.sample,
  };
  handler_(external_latency, trace_info);
  latency_.ping.update(latency.sample);
}

void WebSocketPrivate::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void WebSocketPrivate::parse(const std::string_view &message) {
  profile_.parse([&]() {
    server::TraceInfo trace_info;
    core::json::Buffer buffer(decode_buffer_);
    auto result = json::ParserPrivate::dispatch(*this, message, buffer, trace_info);
  });
}

void WebSocketPrivate::operator()(const json::Error &error, const server::TraceInfo &) {
  LOG(FATAL)("error={}"sv, error);
}

void WebSocketPrivate::operator()(
    const json::SystemStatus &system_status, const server::TraceInfo &) {
  LOG(INFO)("system_status={}"sv, system_status);
}

void WebSocketPrivate::operator()(const json::Pong &pong, const server::TraceInfo &) {
  VLOG(1)("pong={}"sv, pong);
}

void WebSocketPrivate::operator()(const json::Heartbeat &heartbeat, const server::TraceInfo &) {
  VLOG(1)("heartbeat={}"sv, heartbeat);
}

void WebSocketPrivate::operator()(
    const json::SubscriptionStatus &subscription_status, const server::TraceInfo &) {
  LOG(INFO)("subscription_status={}"sv, subscription_status);
}

void WebSocketPrivate::operator()(
    const json::AddOrderStatus &add_order_status, const server::TraceInfo &trace_info) {
  LOG(INFO)("add_order_status={}"sv, add_order_status);
  handler_(add_order_status, trace_info);
}

void WebSocketPrivate::operator()(
    const json::CancelOrderStatus &cancel_order_status, const server::TraceInfo &trace_info) {
  LOG(INFO)("cancel_order_status={}"sv, cancel_order_status);
  handler_(cancel_order_status, trace_info);
}

void WebSocketPrivate::operator()(
    const json::OpenOrders &open_orders, const server::TraceInfo &trace_info) {
  handler_(open_orders, trace_info);
}

void WebSocketPrivate::operator()(
    const json::OwnTrades &own_trades, const server::TraceInfo &trace_info) {
  handler_(own_trades, trace_info);
}

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/web_socket_public.h"

#include <fmt/format.h>

#include "roq/core/clock.h"

#include "roq/kraken/options.h"

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "ws_public";

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

WebSocketPublic::WebSocketPublic(
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
          core::URI(FLAGS_ws_public_uri),
          std::string_view(),  // query
          std::chrono::seconds{FLAGS_ws_public_ping_freq_secs},
          FLAGS_decode_buffer_size,  // XXX need read buffer size
          FLAGS_encode_buffer_size,
          []() { return std::string(); }),
      decode_buffer_(FLAGS_decode_buffer_size),
      counter_{
          .disconnect = create_counter("disconnect"),
      },
      profile_{
          .parse = create_profile("parse"),
      },
      latency_{
          .ping = create_latency("ping"),
          .heartbeat = create_latency("heartbeat"),
      } {
}

bool WebSocketPublic::ready() const {
  return connection_.ready();
}

void WebSocketPublic::close() {
  connection_.close();
}

void WebSocketPublic::operator()(const Event<Start> &) {
  connection_.start();
}

void WebSocketPublic::operator()(const Event<Stop> &) {
  connection_.stop();
}

void WebSocketPublic::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void WebSocketPublic::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

template <>
void WebSocketPublic::subscribe(
    const std::string_view &name, const roq::span<std::string> &pairs) {
  LOG(INFO)(R"(subscribe name="{}", len(pairs)={})", name, std::size(pairs));
  if (FLAGS_ws_public_subscribe_book_depth && name.compare("book") == 0) {
    auto message = fmt::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}",)"
        R"("depth":{})"
        R"(}})"
        R"(}})",
        fmt::join(pairs, R"(",")"),
        name,
        FLAGS_ws_public_subscribe_book_depth);
    DLOG(INFO)(R"(request="{}")", message);
    connection_.send_text(message);
  } else {
    auto message = fmt::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}")"
        R"(}})"
        R"(}})",
        fmt::join(pairs, R"(",")"),
        name);
    VLOG(3)(R"(request="{}")", message);
    connection_.send_text(message);
  }
}

void WebSocketPublic::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void WebSocketPublic::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  handler_(*this);
}

void WebSocketPublic::operator()(const core::web::Socket::Ready &) {
  LOG(INFO)("Ready");
  handler_(*this);
}

void WebSocketPublic::operator()(const core::web::Socket::Close &) {
}

void WebSocketPublic::operator()(const core::web::Socket::Latency &latency) {
  latency_.ping.update(
      std::chrono::duration_cast<std::chrono::nanoseconds>(latency.sample)
          .count());
}

void WebSocketPublic::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void WebSocketPublic::parse(const std::string_view &message) {
  profile_.parse([&]() {
    server::TraceInfo trace_info;
    core::json::Buffer buffer(decode_buffer_);
    auto result =
        json::ParserPublic::dispatch(*this, message, buffer, trace_info);
  });
}

void WebSocketPublic::operator()(
    const json::Error &error, const server::TraceInfo &) {
  LOG(FATAL)("error={}", error);
}

void WebSocketPublic::operator()(
    const json::SystemStatus &system_status, const server::TraceInfo &) {
  LOG(INFO)("system_status={}", system_status);
}

void WebSocketPublic::operator()(
    const json::Pong &pong, const server::TraceInfo &) {
  VLOG(1)("pong={}", pong);
}

void WebSocketPublic::operator()(
    const json::Heartbeat &heartbeat, const server::TraceInfo &) {
  VLOG(1)("heartbeat={}", heartbeat);
}

void WebSocketPublic::operator()(
    const json::SubscriptionStatus &subscription_status,
    const server::TraceInfo &) {
  VLOG(1)("subscription_status={}", subscription_status);
}

void WebSocketPublic::operator()(
    const json::Trade &trade,
    const std::string_view &pair,
    const server::TraceInfo &trace_info) {
  VLOG(3)(R"(trade={}, pair="{}")", trade, pair);
  handler_(trade, pair, trace_info);
}

void WebSocketPublic::operator()(
    const json::Spread &spread,
    const std::string_view &pair,
    const server::TraceInfo &trace_info) {
  VLOG(3)(R"(spread={}, pair="{}")", spread, pair);
  handler_(spread, pair, trace_info);
}

void WebSocketPublic::operator()(
    const json::Book &book,
    const std::string_view &pair,
    const server::TraceInfo &trace_info) {
  VLOG(3)(R"(book={}, pair="{}")", book, pair);
  handler_(book, pair, trace_info);
}

}  // namespace kraken
}  // namespace roq

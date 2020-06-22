/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/web_socket_public.h"

#include <fmt/format.h>

#include "roq/core/clock.h"

#include "roq/kraken/options.h"

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "ws_public";

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

WebSocketPublic::WebSocketPublic(
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
          core::URI(FLAGS_ws_public_uri),
          std::string_view(),  // query
          std::chrono::seconds { FLAGS_ws_public_ping_freq_secs },
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

bool WebSocketPublic::ready() const {
  return _connection.ready();
}

void WebSocketPublic::close() {
  _connection.close();
}

void WebSocketPublic::operator()(const server::StartEvent&) {
  _connection.start();
}

void WebSocketPublic::operator()(const server::StopEvent&) {
  _connection.stop();
}

void WebSocketPublic::operator()(const server::TimerEvent& event) {
  _connection.refresh(event.now);
}

void WebSocketPublic::operator()(metrics::Writer& writer) {
  writer
    // counter
    .write(_counter.disconnect, metrics::COUNTER)
    // profile
    .write(_profile.parse, metrics::PROFILE)
    // latency
    .write(_latency.ping, metrics::LATENCY)
    .write(_latency.heartbeat, metrics::LATENCY);
}

template <>
void WebSocketPublic::subscribe(
    const std::string_view& name,
    const roq::span<std::string>& pairs) {
  LOG(INFO)(
      FMT_STRING(R"(subscribe name="{}", len(pairs)={})"),
      name,
      std::size(pairs));
  if (FLAGS_ws_public_book_depth && name.compare("book") == 0) {
    auto message = fmt::format(
        FMT_STRING(
          R"({{)"
          R"("event":"subscribe",)"
          R"("pair":["{}"],)"
          R"("subscription":{{)"
          R"("name":"{}",)"
          R"("depth":{})"
          R"(}})"
          R"(}})"),
          fmt::join(pairs, R"(",")"),
          name,
          FLAGS_ws_public_book_depth);
    DLOG(INFO)(
        FMT_STRING(R"(request="{}")"),
        message);
    _connection.send_text(message);
  } else {
    auto message = fmt::format(
        FMT_STRING(
          R"({{)"
          R"("event":"subscribe",)"
          R"("pair":["{}"],)"
          R"("subscription":{{)"
          R"("name":"{}")"
          R"(}})"
          R"(}})"),
          fmt::join(pairs, R"(",")"),
          name);
    VLOG(3)(
        FMT_STRING(R"(request="{}")"),
        message);
    _connection.send_text(message);
  }
}

void WebSocketPublic::operator()(const core::web::Socket::Connected&) {
  // note! wait for upgrade
}

void WebSocketPublic::operator()(const core::web::Socket::Disconnected&) {
  ++_counter.disconnect;
  _handler(*this);
}

void WebSocketPublic::operator()(const core::web::Socket::Ready&) {
  LOG(INFO)("Ready");
  _handler(*this);
}

void WebSocketPublic::operator()(const core::web::Socket::Close&) {
}

void WebSocketPublic::operator()(const core::web::Socket::Latency& latency) {
  _latency.ping.update(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          latency.sample).count());
}

void WebSocketPublic::operator()(const core::web::Socket::Text& text) {
  parse(text.payload);
}

void WebSocketPublic::parse(const std::string_view& message) {
  _profile.parse(
      [&]() {
        server::Trace trace;
        core::json::Buffer buffer(_decode_buffer);
        auto result = json::ParserPublic::dispatch(
            *this,
            message,
            buffer,
            trace);
      });
}

void WebSocketPublic::operator()(
    const json::Error& error,
    const server::Trace&) {
  LOG(FATAL)(
      FMT_STRING("error={}"),
      error);
}

void WebSocketPublic::operator()(
    const json::SystemStatus& system_status,
    const server::Trace&) {
  LOG(INFO)(
      FMT_STRING("system_status={}"),
      system_status);
}

void WebSocketPublic::operator()(
    const json::Pong& pong,
    const server::Trace&) {
  VLOG(1)(
      FMT_STRING("pong={}"),
      pong);
}

void WebSocketPublic::operator()(
    const json::Heartbeat& heartbeat,
    const server::Trace&) {
  VLOG(1)(
      FMT_STRING("heartbeat={}"),
      heartbeat);
}

void WebSocketPublic::operator()(
    const json::SubscriptionStatus& subscription_status,
    const server::Trace&) {
  VLOG(1)(
      FMT_STRING("subscription_status={}"),
      subscription_status);
}

void WebSocketPublic::operator()(
    const json::Trade& trade,
    const std::string_view& pair,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(trade={}, pair="{}")"),
      trade,
      pair);
  _handler(
      trade,
      pair,
      trace);
}

void WebSocketPublic::operator()(
    const json::Spread& spread,
    const std::string_view& pair,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(spread={}, pair="{}")"),
      spread,
      pair);
  _handler(
      spread,
      pair,
      trace);
}

void WebSocketPublic::operator()(
    const json::Book& book,
    const std::string_view& pair,
    const server::Trace& trace) {
  VLOG(3)(
      FMT_STRING(R"(book={}, pair="{}")"),
      book,
      pair);
  _handler(
      book,
      pair,
      trace);
}

}  // namespace kraken
}  // namespace roq

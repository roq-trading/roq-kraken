/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/web_socket_public.h"

#include "roq/core/clock.h"

#include "roq/core/metrics/factory.h"

#include "roq/kraken/flags.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

namespace {
static const auto CONNECTION = "ws_public"_sv;

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &function)
      : core::metrics::Factory(Flags::name(), CONNECTION, function) {}
};
}  // namespace

WebSocketPublic::WebSocketPublic(Handler &handler, core::io::Context &context)
    : handler_(handler), connection_(
                             *this,
                             context,
                             core::URI(Flags::ws_public_uri()),
                             std::string_view(),  // query
                             Flags::ws_public_ping_freq(),
                             Flags::decode_buffer_size(),  // XXX need read buffer size
                             Flags::encode_buffer_size(),
                             []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics("disconnect"_sv),
      },
      profile_{
          .parse = create_metrics("parse"_sv),
      },
      latency_{
          .ping = create_metrics("ping"_sv),
          .heartbeat = create_metrics("heartbeat"_sv),
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
void WebSocketPublic::subscribe(const std::string_view &name, const roq::span<std::string> &pairs) {
  LOG(INFO)(R"(subscribe name="{}", len(pairs)={})"_fmt, name, std::size(pairs));
  if (Flags::ws_public_subscribe_book_depth() && name.compare("book"_sv) == 0) {
    auto message = roq::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}",)"
        R"("depth":{})"
        R"(}})"
        R"(}})"_fmt,
        roq::join(pairs, R"(",")"_sv),
        name,
        Flags::ws_public_subscribe_book_depth());
    DLOG(INFO)(R"(request="{}")"_fmt, message);
    connection_.send_text(message);
  } else {
    auto message = roq::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}")"
        R"(}})"
        R"(}})"_fmt,
        roq::join(pairs, R"(",")"_sv),
        name);
    VLOG(3)(R"(request="{}")"_fmt, message);
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
  LOG(INFO)("Ready"_sv);
  handler_(*this);
}

void WebSocketPublic::operator()(const core::web::Socket::Close &) {
}

void WebSocketPublic::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .name = CONNECTION,
      .latency = latency.sample,
  };
  handler_(external_latency, trace_info);
  latency_.ping.update(latency.sample);
}

void WebSocketPublic::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void WebSocketPublic::parse(const std::string_view &message) {
  profile_.parse([&]() {
    server::TraceInfo trace_info;
    core::json::Buffer buffer(decode_buffer_);
    auto result = json::ParserPublic::dispatch(*this, message, buffer, trace_info);
  });
}

void WebSocketPublic::operator()(const json::Error &error, const server::TraceInfo &) {
  LOG(FATAL)("error={}"_fmt, error);
}

void WebSocketPublic::operator()(
    const json::SystemStatus &system_status, const server::TraceInfo &) {
  LOG(INFO)("system_status={}"_fmt, system_status);
}

void WebSocketPublic::operator()(const json::Pong &pong, const server::TraceInfo &) {
  VLOG(1)("pong={}"_fmt, pong);
}

void WebSocketPublic::operator()(const json::Heartbeat &heartbeat, const server::TraceInfo &) {
  VLOG(1)("heartbeat={}"_fmt, heartbeat);
}

void WebSocketPublic::operator()(
    const json::SubscriptionStatus &subscription_status, const server::TraceInfo &) {
  VLOG(1)("subscription_status={}"_fmt, subscription_status);
}

void WebSocketPublic::operator()(
    const json::Trade &trade, const std::string_view &pair, const server::TraceInfo &trace_info) {
  VLOG(3)(R"(trade={}, pair="{}")"_fmt, trade, pair);
  handler_(trade, pair, trace_info);
}

void WebSocketPublic::operator()(
    const json::Spread &spread, const std::string_view &pair, const server::TraceInfo &trace_info) {
  VLOG(3)(R"(spread={}, pair="{}")"_fmt, spread, pair);
  handler_(spread, pair, trace_info);
}

void WebSocketPublic::operator()(
    const json::Book &book, const std::string_view &pair, const server::TraceInfo &trace_info) {
  VLOG(3)(R"(book={}, pair="{}")"_fmt, book, pair);
  handler_(book, pair, trace_info);
}

}  // namespace kraken
}  // namespace roq

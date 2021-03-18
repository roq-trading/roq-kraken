/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/drop_copy.h"

#include "roq/mask.h"
#include "roq/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/kraken/flags.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

namespace {
static const auto NAME = "ex"_sv;
static const auto SUPPORTS = Mask<SupportType>{};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

DropCopy::DropCopy(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared,
    const std::string_view &token)
    : handler_(handler), stream_id_(stream_id),
      name_(roq::format("{}:{}:{}"_fmt, stream_id_, NAME, security.get_account())), token_(token),
      connection_(
          *this,
          context,
          core::URI(Flags::ws_private_uri()),
          {},  // query
          Flags::ws_private_ping_freq(),
          Flags::decode_buffer_size(),  // XXX need read buffer size
          Flags::encode_buffer_size(),
          []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
          .heartbeat = create_metrics(name_, "heartbeat"_sv),
      },
      security_(security), shared_(shared),
      download_(
          Flags::ws_private_request_timeout(), [this](auto state) { return download(state); }) {
}

void DropCopy::operator()(const Event<Start> &) {
  connection_.start();
}

void DropCopy::operator()(const Event<Stop> &) {
  connection_.stop();
}

void DropCopy::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void DropCopy::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void DropCopy::subscribe() {
  subscribe("ownTrades"_sv);
  subscribe("openOrders"_sv);
}

void DropCopy::subscribe(const std::string_view &name) {
  LOG(INFO)(R"(subscribe name="{}", token="{}")"_fmt, name, token_);
  assert(!token_.empty());
  auto message = roq::format(
      R"({{)"
      R"("event":"subscribe",)"
      R"("subscription":{{)"
      R"("name":"{}",)"
      R"("token":"{}")"
      R"(}})"
      R"(}})"_fmt,
      name,
      token_);
  VLOG(3)(R"(request="{}")"_fmt, message);
  connection_.send_text(message);
}

void DropCopy::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void DropCopy::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(GatewayStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(const core::web::Socket::Ready &) {
  (*this)(GatewayStatus::DOWNLOADING);
  download_.begin();
}

void DropCopy::operator()(const core::web::Socket::Close &) {
}

void DropCopy::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .name = name_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void DropCopy::operator()(GatewayStatus status) {
  if (update(status_, status)) {
    server::TraceInfo trace_info;
    StreamUpdate stream_update{
        .stream_id = stream_id_,
        .type = StreamType::WEB_SOCKET,
        .supports = SUPPORTS.get(),
        .account = security_.get_account(),
        .priority = Priority::PRIMARY,
        .status = status_,
    };
    LOG(INFO)("stream_update={}"_fmt, stream_update);
    server::create_trace_and_dispatch(trace_info, stream_update, handler_);
  }
}

uint32_t DropCopy::download(DropCopyState state) {
  switch (state) {
    case DropCopyState::UNDEFINED:
      assert(false);
      break;
    case DropCopyState::SUBSCRIBE:
      subscribe();
      return {};
    case DropCopyState::DONE:
      (*this)(GatewayStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void DropCopy::parse(const std::string_view &message) {
  profile_.parse([&]() {
    server::TraceInfo trace_info;
    core::json::Buffer buffer(decode_buffer_);
    auto result = json::ParserPrivate::dispatch(*this, message, buffer, trace_info);
    LOG_IF(WARNING, !result)(R"(Unexpected: message="{}")"_fmt, message);
  });
}

void DropCopy::operator()(const json::Error &error, const server::TraceInfo &) {
  LOG(FATAL)("error={}"_fmt, error);
}

void DropCopy::operator()(const json::SystemStatus &system_status, const server::TraceInfo &) {
  LOG(INFO)("system_status={}"_fmt, system_status);
}

void DropCopy::operator()(const json::Pong &pong, const server::TraceInfo &) {
  VLOG(1)("pong={}"_fmt, pong);
}

void DropCopy::operator()(const json::Heartbeat &heartbeat, const server::TraceInfo &) {
  VLOG(1)("heartbeat={}"_fmt, heartbeat);
}

void DropCopy::operator()(
    const json::SubscriptionStatus &subscription_status, const server::TraceInfo &) {
  LOG(INFO)("subscription_status={}"_fmt, subscription_status);
}

void DropCopy::operator()(const json::AddOrderStatus &add_order_status, const server::TraceInfo &) {
  LOG(INFO)("add_order_status={}"_fmt, add_order_status);
  LOG(FATAL)("NOT IMPLEMENTED");
}

void DropCopy::operator()(
    const json::CancelOrderStatus &cancel_order_status, const server::TraceInfo &) {
  LOG(INFO)("cancel_order_status={}"_fmt, cancel_order_status);
  LOG(FATAL)("NOT IMPLEMENTED");
}

void DropCopy::operator()(const json::OpenOrders &open_orders, const server::TraceInfo &) {
  LOG(INFO)("open_orders={}"_fmt, open_orders);
  LOG(FATAL)("NOT IMPLEMENTED");
}

void DropCopy::operator()(const json::OwnTrades &own_trades, const server::TraceInfo &) {
  LOG(INFO)("own_trades={}"_fmt, own_trades);
  LOG(FATAL)("NOT IMPLEMENTED");
}

}  // namespace kraken
}  // namespace roq

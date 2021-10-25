/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/drop_copy.h"

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/kraken/flags.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

namespace {
static const auto NAME = "ex"_sv;
static const auto SUPPORTS = utils::Mask<SupportType>{};

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
      name_(fmt::format("{}:{}:{}"_sv, stream_id_, NAME, security.get_account())), token_(token),
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
  log::info(R"(subscribe name="{}", token="{}")"_sv, name, token_);
  assert(!token_.empty());
  auto message = fmt::format(
      R"({{)"
      R"("event":"subscribe",)"
      R"("subscription":{{)"
      R"("name":"{}",)"
      R"("token":"{}")"
      R"(}})"
      R"(}})"_sv,
      name,
      token_);
  log::info<3>(R"(request="{}")"_sv, message);
  connection_.send_text(message);
}

void DropCopy::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void DropCopy::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(const core::web::Socket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void DropCopy::operator()(const core::web::Socket::Close &) {
}

void DropCopy::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void DropCopy::operator()(const core::web::Socket::Binary &) {
  log::fatal("Unexpected"_sv);
}

void DropCopy::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    server::TraceInfo trace_info;
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_sv, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
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
      (*this)(ConnectionStatus::READY);
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
    if (ROQ_UNLIKELY(!result))
      log::warn(R"(Unexpected: message="{}")"_sv, message);
  });
}

void DropCopy::operator()(const server::Trace<json::Error> &event) {
  auto &[trace_info, error] = event;
  log::fatal("error={}"_sv, error);
}

void DropCopy::operator()(const server::Trace<json::SystemStatus> &event) {
  auto &[trace_info, system_status] = event;
  log::info("system_status={}"_sv, system_status);
}

void DropCopy::operator()(const server::Trace<json::Pong> &event) {
  auto &[trace_info, pong] = event;
  log::info<1>("pong={}"_sv, pong);
}

void DropCopy::operator()(const server::Trace<json::Heartbeat> &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<1>("heartbeat={}"_sv, heartbeat);
}

void DropCopy::operator()(const server::Trace<json::SubscriptionStatus> &event) {
  auto &[trace_info, subscription_status] = event;
  log::info("subscription_status={}"_sv, subscription_status);
}

void DropCopy::operator()(const server::Trace<json::AddOrderStatus> &event) {
  auto &[trace_info, add_order_status] = event;
  log::info("add_order_status={}"_sv, add_order_status);
  throw NotImplementedException();
}

void DropCopy::operator()(const server::Trace<json::CancelOrderStatus> &event) {
  auto &[trace_info, cancel_order_status] = event;
  log::info("cancel_order_status={}"_sv, cancel_order_status);
  throw NotImplementedException();
}

void DropCopy::operator()(const server::Trace<json::OpenOrders> &event) {
  auto &[trace_info, open_orders] = event;
  log::info("open_orders={}"_sv, open_orders);
  throw NotImplementedException();
}

void DropCopy::operator()(const server::Trace<json::OwnTrades> &event) {
  auto &[trace_info, own_trades] = event;
  log::info("own_trades={}"_sv, own_trades);
  throw NotImplementedException();
}

}  // namespace kraken
}  // namespace roq

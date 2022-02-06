/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/drop_copy.h"

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/metrics/factory.h"

#include "roq/kraken/flags.h"

using namespace std::literals;

namespace roq {
namespace kraken {

namespace {
const auto NAME = "ex"sv;
const auto SUPPORTS = utils::Mask<SupportType>{};

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
      name_(fmt::format("{}:{}:{}"sv, stream_id_, NAME, security.get_account())), token_(token),
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
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
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
  subscribe("ownTrades"sv);
  subscribe("openOrders"sv);
}

void DropCopy::subscribe(const std::string_view &name) {
  log::info(R"(subscribe name="{}", token="{}")"sv, name, token_);
  assert(!std::empty(token_));
  auto message = fmt::format(
      R"({{)"
      R"("event":"subscribe",)"
      R"("subscription":{{)"
      R"("name":"{}",)"
      R"("token":"{}")"
      R"(}})"
      R"(}})"sv,
      name,
      token_);
  log::info<3>(R"(request="{}")"sv, message);
  connection_.send_text(message);
}

void DropCopy::operator()(const core::web::ClientSocket::Connected &) {
  // note! wait for upgrade
}

void DropCopy::operator()(const core::web::ClientSocket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(const core::web::ClientSocket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void DropCopy::operator()(const core::web::ClientSocket::Close &) {
}

void DropCopy::operator()(const core::web::ClientSocket::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = security_.get_account(),
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(const core::web::ClientSocket::Text &text) {
  parse(text.payload);
}

void DropCopy::operator()(const core::web::ClientSocket::Binary &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
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
    auto trace_info = server::create_trace_info();
    core::json::Buffer buffer(decode_buffer_);
    auto result = json::ParserPrivate::dispatch(*this, message, buffer, trace_info);
    if (!result) [[unlikely]]
      log::warn(R"(Unexpected: message="{}")"sv, message);
  });
}

void DropCopy::operator()(const server::Trace<json::Error> &event) {
  auto &[trace_info, error] = event;
  log::fatal("error={}"sv, error);
}

void DropCopy::operator()(const server::Trace<json::SystemStatus> &event) {
  auto &[trace_info, system_status] = event;
  log::info("system_status={}"sv, system_status);
}

void DropCopy::operator()(const server::Trace<json::Pong> &event) {
  auto &[trace_info, pong] = event;
  log::info<1>("pong={}"sv, pong);
}

void DropCopy::operator()(const server::Trace<json::Heartbeat> &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<1>("heartbeat={}"sv, heartbeat);
}

void DropCopy::operator()(const server::Trace<json::SubscriptionStatus> &event) {
  auto &[trace_info, subscription_status] = event;
  log::info("subscription_status={}"sv, subscription_status);
}

void DropCopy::operator()(const server::Trace<json::AddOrderStatus> &event) {
  auto &[trace_info, add_order_status] = event;
  log::info("add_order_status={}"sv, add_order_status);
  throw NotImplemented("not implemented"sv);
}

void DropCopy::operator()(const server::Trace<json::CancelOrderStatus> &event) {
  auto &[trace_info, cancel_order_status] = event;
  log::info("cancel_order_status={}"sv, cancel_order_status);
  throw NotImplemented("not implemented"sv);
}

void DropCopy::operator()(const server::Trace<json::OpenOrders> &event) {
  auto &[trace_info, open_orders] = event;
  log::info("open_orders={}"sv, open_orders);
  throw NotImplemented("not implemented"sv);
}

void DropCopy::operator()(const server::Trace<json::OwnTrades> &event) {
  auto &[trace_info, own_trades] = event;
  log::info("own_trades={}"sv, own_trades);
  throw NotImplemented("not implemented"sv);
}

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/kraken/drop_copy.hpp"

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/socket/client.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === CONSTANTS ===

namespace {
auto const NAME = "ex"sv;

auto const SUPPORTS = Mask<SupportType>{};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.private_uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = {},
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.private_ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,  // XXX need read buffer size
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() -> std::string { return {}; });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

DropCopy::DropCopy(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared, std::string_view const &token)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account.name)}, token_{token},
      connection_{create_connection(*this, shared.settings, context)}, decode_buffer_(shared.settings.misc.decode_buffer_size),
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(shared.settings, name_, "parse"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
          .heartbeat = create_metrics(shared.settings, name_, "heartbeat"sv),
      },
      account_{account}, shared_{shared}, download_{shared.settings.ws.private_request_timeout, [this](auto state) { return download(state); }} {
}

void DropCopy::operator()(Event<Start> const &) {
  (*connection_).start();
}

void DropCopy::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void DropCopy::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
}

void DropCopy::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void DropCopy::subscribe() {
  subscribe("ownTrades"sv);
  subscribe("openOrders"sv);
}

void DropCopy::subscribe(std::string_view const &name) {
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
  (*connection_).send_text(message);
}

void DropCopy::operator()(web::socket::Client::Connected const &) {
  // note! wait for upgrade
}

void DropCopy::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void DropCopy::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void DropCopy::operator()(web::socket::Client::Close const &) {
}

void DropCopy::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void DropCopy::operator()(web::socket::Client::Text const &text) {
  parse(text.payload);
}

void DropCopy::operator()(web::socket::Client::Binary const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = account_.name,
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::WS,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
        .interface = (*connection_).get_interface(),
        .authority = (*connection_).get_current_authority(),
        .path = (*connection_).get_current_path(),
        .proxy = (*connection_).get_proxy(),
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

uint32_t DropCopy::download(DropCopyState state) {
  switch (state) {
    using enum DropCopyState;
    case UNDEFINED:
      assert(false);
      break;
    case SUBSCRIBE:
      subscribe();
      return 0;
    case DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return 0;
  }
  assert(false);
  return 0;
}

void DropCopy::parse(std::string_view const &message) {
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(message="{}")"sv, message); };
    TraceInfo trace_info;
    try {
      if (!json::ParserPrivate::dispatch(*this, message, decode_buffer_, trace_info))
        log_message();
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

void DropCopy::operator()(Trace<json::Error> const &event) {
  auto &[trace_info, error] = event;
  log::fatal("error={}"sv, error);
}

void DropCopy::operator()(Trace<json::SystemStatus> const &event) {
  auto &[trace_info, system_status] = event;
  log::info("system_status={}"sv, system_status);
}

void DropCopy::operator()(Trace<json::Pong> const &event) {
  auto &[trace_info, pong] = event;
  log::info<1>("pong={}"sv, pong);
}

void DropCopy::operator()(Trace<json::Heartbeat> const &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<1>("heartbeat={}"sv, heartbeat);
}

void DropCopy::operator()(Trace<json::SubscriptionStatus> const &event) {
  auto &[trace_info, subscription_status] = event;
  log::info("subscription_status={}"sv, subscription_status);
}

void DropCopy::operator()(Trace<json::AddOrderStatus> const &event) {
  auto &[trace_info, add_order_status] = event;
  log::info("add_order_status={}"sv, add_order_status);
  throw NotImplemented{"not implemented"sv};
}

void DropCopy::operator()(Trace<json::CancelOrderStatus> const &event) {
  auto &[trace_info, cancel_order_status] = event;
  log::info("cancel_order_status={}"sv, cancel_order_status);
  throw NotImplemented{"not implemented"sv};
}

void DropCopy::operator()(Trace<json::OpenOrders> const &event) {
  auto &[trace_info, open_orders] = event;
  log::info("open_orders={}"sv, open_orders);
  throw NotImplemented{"not implemented"sv};
}

void DropCopy::operator()(Trace<json::OwnTrades> const &event) {
  auto &[trace_info, own_trades] = event;
  log::info("own_trades={}"sv, own_trades);
  throw NotImplemented{"not implemented"sv};
}

}  // namespace kraken
}  // namespace roq

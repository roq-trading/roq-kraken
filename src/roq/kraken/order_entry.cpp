/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/order_entry.hpp"

#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/kraken/flags.hpp"

#include "roq/kraken/json/result.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

namespace {
const auto NAME = "om"sv;

const auto SUPPORTS = Mask{
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::rest_uri();
  core::web::Client::Config config{
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
      .validate_certificate = server::Flags::tls_validate_certificate(),
      .uris = {&uri, 1},
      .proxy = Flags::rest_proxy(),
      .user_agent = ROQ_PACKAGE_NAME,
      .connection = core::http::Connection::KEEP_ALIVE,
      .allow_pipelining = true,
      .request_timeout = Flags::rest_request_timeout(),
      .ping_frequency = Flags::rest_ping_freq(),
      .ping_path = Flags::rest_ping_path(),
  };
  return core::web::Client{handler, context, config};
}
}  // namespace

OrderEntry::OrderEntry(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared)
    : handler_(handler), stream_id_(stream_id),
      name_(fmt::format("{}:{}:{}"sv, stream_id_, NAME, security.get_account())),
      connection_(create_connection(*this, context)), decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .get_web_sockets_token = create_metrics(name_, "get_web_sockets_token"sv),
          .get_web_sockets_token_ack = create_metrics(name_, "get_web_sockets_token_ack"sv),
          .positions = create_metrics(name_, "positions"sv),
          .positions_ack = create_metrics(name_, "positions_ack"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      security_(security), shared_(shared),
      download_(Flags::rest_request_timeout(), [this](auto state) { return download(state); }) {
}

void OrderEntry::operator()(const Event<Start> &) {
  connection_.start();
}

void OrderEntry::operator()(const Event<Stop> &) {
  connection_.stop();
}

void OrderEntry::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void OrderEntry::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.get_web_sockets_token, metrics::PROFILE)
      .write(profile_.get_web_sockets_token_ack, metrics::PROFILE)
      .write(profile_.positions, metrics::PROFILE)
      .write(profile_.positions_ack, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

uint16_t OrderEntry::operator()(
    const Event<CreateOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id) {
  throw NotImplemented("not implemented"sv);
}

uint16_t OrderEntry::operator()(
    const Event<ModifyOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  throw NotImplemented("not implemented"sv);
}

uint16_t OrderEntry::operator()(
    const Event<CancelOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  throw NotImplemented("not implemented"sv);
}

uint16_t OrderEntry::operator()(
    const Event<CancelAllOrders> &, [[maybe_unused]] const std::string_view &request_id) {
  log::warn("*** CANCEL ALL ORDERS *NOT* SUPPORTED ***"sv);
  return stream_id_;
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::REST,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void OrderEntry::operator()(const core::web::Client::Connected &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void OrderEntry::operator()(const core::web::Client::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void OrderEntry::operator()(const core::web::Client::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = security_.get_account(),
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    case OrderEntryState::UNDEFINED:
      assert(false);
      break;
    case OrderEntryState::TOKEN:
      get_token();
      return 1;
    case OrderEntryState::POSITIONS:
      get_positions();
      return 1;
    case OrderEntryState::DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// token

void OrderEntry::get_token() {
  profile_.get_web_sockets_token([&]() {
    auto method = core::http::Method::POST;
    auto path = "/0/private/GetWebSocketsToken"sv;
    auto body = security_.create_body();
    auto headers = security_.create_headers(method, path, body);
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = core::http::ContentType::FORM,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
    };
    auto sequence = download_.sequence();
    connection_(
        "token"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          auto trace_info = server::create_trace_info();
          server::Trace event(trace_info, response);
          get_token_ack(event, sequence);
        });
  });
}

void OrderEntry::get_token_ack(const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.get_web_sockets_token([&]() {
    // auto &[trace_info, response] = event;
    auto &trace_info = event.trace_info;
    auto &response = event.value;
    auto state = OrderEntryState::TOKEN;
    try {
      auto [status, category, body] = response.result();
      log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
      if (download_.skip(sequence, state)) {
        log::info("Download state={} has already been processed"sv, state);
        return;
      }
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      json::Result::dispatch<json::Token>(
          body,
          buffer,
          [](const std::span<std::string_view> &e) {  // error
            log::warn("error=[{}]"sv, fmt::join(e, ","sv));
            log::fatal("Unexpected"sv);
          },
          [&](const json::Token &token) {  // success
            server::Trace event(trace_info, token);
            (*this)(event);
          });
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::Token> &event) {
  auto &[trace_info, token] = event;
  log::info<2>(R"(token={})"sv, token);
  TokenUpdate token_update{
      .account = security_.get_account(),
      .token = token.token,
  };
  handler_(token_update);
}

// positions

void OrderEntry::get_positions() {
  profile_.positions([&]() {
    auto method = core::http::Method::POST;
    auto path = "/0/private/OpenPositions"sv;
    auto body = security_.create_body();
    auto headers = security_.create_headers(method, path, body);
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = core::http::ContentType::FORM,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
    };
    auto sequence = download_.sequence();
    connection_(
        "positions"sv,
        request,
        [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          auto trace_info = server::create_trace_info();
          server::Trace event(trace_info, response);
          get_positions_ack(event, sequence);
        });
  });
}

void OrderEntry::get_positions_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.positions_ack([&]() {
    auto &[trace_info, response] = event;
    auto state = OrderEntryState::POSITIONS;
    try {
      auto [status, category, body] = response.result();
      log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
      if (download_.skip(sequence, state)) {
        log::info("Download state={} has already been processed"sv, state);
        return;
      }
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      auto positions = core::json::Parser::create<json::Positions>(body, buffer);
      server::Trace event(trace_info, positions);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::Positions> &event) {
  auto &[trace_info, positions] = event;
  log::info<4>("positions={}"sv, positions);
  assert(std::empty(positions.error));
}

}  // namespace kraken
}  // namespace roq

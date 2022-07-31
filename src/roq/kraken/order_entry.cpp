/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/order_entry.hpp"

#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/rest/client_factory.hpp"

#include "roq/kraken/flags.hpp"

#include "roq/kraken/json/result.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

namespace {
auto const NAME = "om"sv;

const Mask SUPPORTS{
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(std::string_view const &group, std::string_view const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::rest_uri();
  web::rest::Client::Config config{
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
      .validate_certificate = server::Flags::net_tls_validate_certificate(),
      .uris = {&uri, 1},
      .proxy = Flags::rest_proxy(),
      .user_agent = ROQ_PACKAGE_NAME,
      .connection = web::http::Connection::KEEP_ALIVE,
      .allow_pipelining = true,
      .request_timeout = Flags::rest_request_timeout(),
      .ping_frequency = Flags::rest_ping_freq(),
      .ping_path = Flags::rest_ping_path(),
  };
  return web::rest::ClientFactory::create(handler, context, config);
}
}  // namespace

OrderEntry::OrderEntry(Handler &handler, io::Context &context, uint16_t stream_id, Security &security, Shared &shared)
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

void OrderEntry::operator()(Event<Start> const &) {
  (*connection_).start();
}

void OrderEntry::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void OrderEntry::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
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
    Event<CreateOrder> const &, oms::Order const &, [[maybe_unused]] std::string_view const &request_id) {
  throw NotImplemented("not implemented"sv);
}

uint16_t OrderEntry::operator()(
    Event<ModifyOrder> const &,
    oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw NotImplemented("not implemented"sv);
}

uint16_t OrderEntry::operator()(
    Event<CancelOrder> const &,
    oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw NotImplemented("not implemented"sv);
}

uint16_t OrderEntry::operator()(Event<CancelAllOrders> const &, [[maybe_unused]] std::string_view const &request_id) {
  log::warn("*** CANCEL ALL ORDERS *NOT* SUPPORTED ***"sv);
  return stream_id_;
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    const StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
    };
    log::info("stream_status={}"sv, stream_status);
    create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void OrderEntry::operator()(web::rest::Client::Connected const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void OrderEntry::operator()(web::rest::Client::Disconnected const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void OrderEntry::operator()(web::rest::Client::Latency const &latency) {
  auto trace_info = server::create_trace_info();
  const ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = security_.get_account(),
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    using enum OrderEntryState;
    case UNDEFINED:
      assert(false);
      break;
    case TOKEN:
      get_token();
      return 1;
    case POSITIONS:
      get_positions();
      return 1;
    case DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// token

void OrderEntry::get_token() {
  profile_.get_web_sockets_token([&]() {
    auto method = web::http::Method::POST;
    auto path = "/0/private/GetWebSocketsToken"sv;
    auto body = security_.create_body();
    auto headers = security_.create_headers(method, path, body);
    web::rest::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = web::http::ContentType::APPLICATION_X_WWW_FORM_URLENCODED,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
    };
    auto sequence = download_.sequence();
    (*connection_)("token"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
      auto trace_info = server::create_trace_info();
      Trace event(trace_info, response);
      get_token_ack(event, sequence);
    });
  });
}

void OrderEntry::get_token_ack(Trace<web::rest::Response const> const &event, uint32_t sequence) {
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
      response.expect(web::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      json::Result::dispatch<json::Token>(
          body,
          buffer,
          [](const std::span<std::string_view> &e) {  // error
            log::warn("error=[{}]"sv, fmt::join(e, ","sv));
            log::fatal("Unexpected"sv);
          },
          [&](const json::Token &token) {  // success
            Trace event(trace_info, token);
            (*this)(event);
          });
      download_.check(state);
    } catch (NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(Trace<json::Token const> const &event) {
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
    auto method = web::http::Method::POST;
    auto path = "/0/private/OpenPositions"sv;
    auto body = security_.create_body();
    auto headers = security_.create_headers(method, path, body);
    web::rest::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = web::http::ContentType::APPLICATION_X_WWW_FORM_URLENCODED,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
    };
    auto sequence = download_.sequence();
    (*connection_)("positions"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
      auto trace_info = server::create_trace_info();
      Trace event(trace_info, response);
      get_positions_ack(event, sequence);
    });
  });
}

void OrderEntry::get_positions_ack(Trace<web::rest::Response const> const &event, uint32_t sequence) {
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
      response.expect(web::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      const auto positions = core::json::Parser::create<json::Positions>(body, buffer);
      Trace event(trace_info, positions);
      (*this)(event);
      download_.check(state);
    } catch (NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(Trace<json::Positions const> const &event) {
  auto &[trace_info, positions] = event;
  log::info<4>("positions={}"sv, positions);
  assert(std::empty(positions.error));
}

}  // namespace kraken
}  // namespace roq

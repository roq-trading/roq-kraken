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

// === CONSTANTS ===

namespace {
auto const NAME = "om"sv;

auto const SUPPORTS = Mask{
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto const &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account);
}

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

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto const &group, auto const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

// === IMPLEMENTATION ===

OrderEntry::OrderEntry(Handler &handler, io::Context &context, uint16_t stream_id, Security &security, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(create_name(stream_id_, security.get_account())),
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
  throw NotImplemented{"not implemented"sv};
}

uint16_t OrderEntry::operator()(
    Event<ModifyOrder> const &,
    oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw NotImplemented{"not implemented"sv};
}

uint16_t OrderEntry::operator()(
    Event<CancelOrder> const &,
    oms::Order const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw NotImplemented{"not implemented"sv};
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
      Trace event{trace_info, response};
      get_token_ack(event, sequence);
    });
  });
}

void OrderEntry::get_token_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  constexpr auto const STATE = OrderEntryState::TOKEN;
  profile_.get_web_sockets_token([&]() {
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        core::json::Buffer buffer{decode_buffer_};
        json::Result::dispatch<json::Token>(
            body,
            buffer,
            [](const std::span<std::string_view> &e) {  // error
              log::warn("error=[{}]"sv, fmt::join(e, ","sv));
              log::fatal("Unexpected"sv);
            },
            [&](const json::Token &token) {  // success
              Trace event_2{event, token};
              (*this)(event_2);
            });
        download_.check(STATE);
      }
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      download_.retry(STATE);
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<json::Token> const &event) {
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
      Trace event{trace_info, response};
      get_positions_ack(event, sequence);
    });
  });
}

void OrderEntry::get_positions_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  constexpr auto const STATE = OrderEntryState::POSITIONS;
  profile_.positions_ack([&]() {
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        core::json::Buffer buffer{decode_buffer_};
        const auto positions = core::json::Parser::create<json::Positions>(body, buffer);
        Trace event_2{event, positions};
        (*this)(event_2);
        download_.check(STATE);
      }
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      download_.retry(STATE);
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<json::Positions> const &event) {
  auto &[trace_info, positions] = event;
  log::info<4>("positions={}"sv, positions);
  assert(std::empty(positions.error));
}

template <typename SuccessHandler, typename ErrorHandler>
void OrderEntry::process_response(
    web::rest::Response const &response, SuccessHandler success_handler, ErrorHandler error_handler) {
  try {
    auto [status, category, body] = response.result();
    log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
    switch (category) {
      using enum web::http::Category;
      case SUCCESS:  // 2xx
        success_handler(body);
        break;
      case CLIENT_ERROR:  // 4xx
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, magic_enum::enum_name(status));
        break;
      case SERVER_ERROR:  // 5xx
        error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, magic_enum::enum_name(status));
        break;
      default:
        response.expect(web::http::Status::OK);  // throws
    }
  } catch (oms::Exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(e.origin, e.status, e.error, e.what());
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

template <typename... Args>
void OrderEntry::operator()(Trace<oms::Response> const &event, uint8_t user_id, uint32_t order_id, Args &&...args) {
  auto &[trace_info, response] = event;
  if (shared_.update_order(
          user_id,
          order_id,
          stream_id_,
          trace_info,
          response,
          std::forward<Args>(args)...,
          []([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("Did not find order: user_id={}, order_id={}"sv, user_id, order_id);
  }
}

template <typename... Args>
void OrderEntry::operator()(
    Trace<oms::OrderUpdate> const &event, std::string_view const &client_order_id, Args &&...args) {
  auto &[trace_info, order_update] = event;
  if (shared_.update_order(
          client_order_id, stream_id_, trace_info, order_update, [&]([[maybe_unused]] auto &order) {})) {
  } else {
    log::warn("*** EXTERNAL ORDER ***"sv);
  }
}

}  // namespace kraken
}  // namespace roq

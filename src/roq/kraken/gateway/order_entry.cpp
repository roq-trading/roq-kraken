/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/gateway/order_entry.hpp"

#include <utility>

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/utils/charconv/from_chars.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/kraken/protocol/json/map.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace gateway {

// === CONSTANTS ===

namespace {
auto const NAME = "om"sv;

auto const SUPPORTS = Mask{
    SupportType::ORDER,
    SupportType::FUNDS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 1;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id, auto &account) {
  return fmt::format("{}:{}:{}"sv, stream_id, NAME, account);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.rest.uri;
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .proxy = settings.rest.proxy,
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = settings.net.disconnect_on_idle_timeout,
      .connection = web::http::Connection::KEEP_ALIVE,
      // request
      .allow_pipelining = true,
      .request_timeout = settings.rest.request_timeout,
      // response
      .suspend_on_retry_after = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .ping_frequency = settings.rest.ping_freq,
      .ping_path = settings.rest.ping_path,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::rest::Client::create(handler, context, config);
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

OrderEntry::OrderEntry(Handler &handler, io::Context &context, uint16_t stream_id, Account &account, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_, account.name)}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .get_web_sockets_token = create_metrics(shared.settings, name_, "get_web_sockets_token"sv),
          .get_web_sockets_token_ack = create_metrics(shared.settings, name_, "get_web_sockets_token_ack"sv),
          .balance = create_metrics(shared.settings, name_, "balance"sv),
          .balance_ack = create_metrics(shared.settings, name_, "balance_ack"sv),
          .trade_balance = create_metrics(shared.settings, name_, "trade_balance"sv),
          .trade_balance_ack = create_metrics(shared.settings, name_, "trade_balance_ack"sv),
          .open_positions = create_metrics(shared.settings, name_, "open_positions"sv),
          .open_positions_ack = create_metrics(shared.settings, name_, "open_positions_ack"sv),
          .open_orders = create_metrics(shared.settings, name_, "open_orders"sv),
          .open_orders_ack = create_metrics(shared.settings, name_, "open_orders_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      account_{account}, shared_{shared}, download_{shared.settings.rest.request_timeout, [this](auto state) { return download(state); }} {
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

void OrderEntry::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.get_web_sockets_token, metrics::Type::PROFILE)
      .write(profile_.get_web_sockets_token_ack, metrics::Type::PROFILE)
      .write(profile_.balance, metrics::Type::PROFILE)
      .write(profile_.balance_ack, metrics::Type::PROFILE)
      .write(profile_.trade_balance, metrics::Type::PROFILE)
      .write(profile_.trade_balance_ack, metrics::Type::PROFILE)
      .write(profile_.open_positions, metrics::Type::PROFILE)
      .write(profile_.open_positions_ack, metrics::Type::PROFILE)
      .write(profile_.open_orders, metrics::Type::PROFILE)
      .write(profile_.open_orders_ack, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY);
}

uint16_t OrderEntry::operator()(
    Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, [[maybe_unused]] std::string_view const &request_id) {
  throw NotImplemented{"not implemented"sv};
}

uint16_t OrderEntry::operator()(
    Event<ModifyOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw NotImplemented{"not implemented"sv};
}

uint16_t OrderEntry::operator()(
    Event<CancelOrder> const &,
    server::oms::Order const &,
    server::oms::RefData const &,
    [[maybe_unused]] std::string_view const &request_id,
    [[maybe_unused]] std::string_view const &previous_request_id) {
  throw NotImplemented{"not implemented"sv};
}

uint16_t OrderEntry::operator()(Event<CancelAllOrders> const &, [[maybe_unused]] std::string_view const &request_id) {
  throw server::oms::NotSupported{"CancelAllOrders"sv};
  return stream_id_;
}

void OrderEntry::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = account_.name,
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::HTTP,
      .encoding = {Encoding::JSON},
      .priority = Priority::PRIMARY,
      .connection_status = connection_status_,
      .reason = reason,
      .interface = (*connection_).get_interface(),
      .authority = (*connection_).get_current_authority(),
      .path = (*connection_).get_current_path(),
      .proxy = (*connection_).get_proxy(),
  };
  log::info("stream_status={}"sv, stream_status);
  create_trace_and_dispatch(shared_.dispatcher, trace_info, stream_status);
}

void OrderEntry::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    download_.begin();
  }
}

void OrderEntry::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading()) {
    download_.reset();
  }
}

void OrderEntry::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = account_.name,
      .latency = latency.sample,
  };
  create_trace_and_dispatch(shared_.dispatcher, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

uint32_t OrderEntry::download(State state) {
  switch (state) {
    using enum State;
    case UNDEFINED:
      assert(false);
      break;
    case TOKEN:
      get_token();
      return 1;
    case BALANCE:
      (*this)(ConnectionStatus::DOWNLOADING, "balance"sv);
      get_balance();
      return 1;
    case TRADE_BALANCE:
      (*this)(ConnectionStatus::DOWNLOADING, "trade-balance"sv);
      get_trade_balance();
      return 1;
    case OPEN_POSITIONS:
      (*this)(ConnectionStatus::DOWNLOADING, "open-positions"sv);
      get_open_positions();
      return 1;
    case OPEN_ORDERS:
      (*this)(ConnectionStatus::DOWNLOADING, "open-orders"sv);
      get_open_orders();
      return 1;
    case DONE:
      (*this)(ConnectionStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

// token

void OrderEntry::get_token() {
  profile_.get_web_sockets_token([&]() {
    auto method = web::http::Method::POST;
    auto path = shared_.api.order_management.get_web_sockets_token;
    auto body = account_.create_body();
    auto headers = account_.create_headers(method, path, body);
    auto request = web::rest::Request{
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
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_token_ack(event, sequence);
    });
  });
}

void OrderEntry::get_token_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::TOKEN;
  profile_.get_web_sockets_token([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto const &text) {
      log::warn(R"(account="{}", origin={}, error={}, status={}, text="{}")"sv, account_.name, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::TokenAck token_ack{body, decode_buffer_};
        if (std::empty(token_ack.error)) {
          Trace event_2{event, token_ack};
          (*this)(event_2);
          download_.check(STATE);
        } else {
          handle_error(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNDEFINED, token_ack.error[0]);
        }
      }
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<protocol::json::TokenAck> const &event) {
  auto &[trace_info, token_ack] = event;
  log::info<2>(R"(token_ack={})"sv, token_ack);
  auto token_update = TokenUpdate{
      .account = account_.name,
      .token = token_ack.result.token,
  };
  handler_(token_update);
}

// balance

void OrderEntry::get_balance() {
  profile_.balance([&]() {
    auto method = web::http::Method::POST;
    auto path = shared_.api.order_management.balance;
    auto body = account_.create_body();
    auto headers = account_.create_headers(method, path, body);
    auto request = web::rest::Request{
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
    (*connection_)("balance"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_balance_ack(event, sequence);
    });
  });
}

void OrderEntry::get_balance_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::BALANCE;
  profile_.balance_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto text) {
      log::warn(R"(account="{}", origin={}, error={}, status={}, text="{}")"sv, account_.name, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::BalanceAck balance_ack{body, decode_buffer_};
        Trace event_2{event, balance_ack};
        (*this)(event_2);
        download_.check(STATE);
      }
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<protocol::json::BalanceAck> const &event) {
  auto &[trace_info, balance_ack] = event;
  log::info<4>("balance_ack={}"sv, balance_ack);
  assert(std::empty(balance_ack.error));
  for (auto &item : balance_ack.result) {
    auto balance = utils::charconv::from_string_relaxed<double>(item.VALUE);
    auto funds_update = FundsUpdate{
        .stream_id = stream_id_,
        .account = account_.name,
        .currency = item.KEY,
        .margin_mode = {},
        .balance = balance,
        .hold = NaN,
        .borrowed = NaN,
        .unrealized_pnl = NaN,
        .external_account = {},
        .update_type = UpdateType::SNAPSHOT,
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, funds_update, true);
  }
}

// trade-balance

void OrderEntry::get_trade_balance() {
  profile_.trade_balance([&]() {
    auto method = web::http::Method::POST;
    auto path = shared_.api.order_management.trade_balance;
    auto body = account_.create_body();
    auto headers = account_.create_headers(method, path, body);
    auto request = web::rest::Request{
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
    (*connection_)("trade_balance"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_trade_balance_ack(event, sequence);
    });
  });
}

void OrderEntry::get_trade_balance_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::TRADE_BALANCE;
  profile_.trade_balance_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto text) {
      log::warn(R"(account="{}", origin={}, error={}, status={}, text="{}")"sv, account_.name, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&]([[maybe_unused]] auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        // XXX FIXME TODO need key-double autogen
        // protocol::json::TradeBalanceAck trade_balance_ack{body, decode_buffer_};
        // Trace event_2{event, trade_balance_ack};
        // (*this)(event_2);
        download_.check(STATE);
      }
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<protocol::json::TradeBalanceAck> const &event) {
  auto &[trace_info, trade_balance_ack] = event;
  log::info<4>("trade_balance_ack={}"sv, trade_balance_ack);
  assert(std::empty(trade_balance_ack.error));
}

// open-positions

void OrderEntry::get_open_positions() {
  profile_.open_positions([&]() {
    auto method = web::http::Method::POST;
    auto path = shared_.api.order_management.open_positions;
    auto body = account_.create_body();
    auto headers = account_.create_headers(method, path, body);
    auto request = web::rest::Request{
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
    (*connection_)("open-positions"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_open_positions_ack(event, sequence);
    });
  });
}

void OrderEntry::get_open_positions_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::OPEN_POSITIONS;
  profile_.open_positions_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto text) {
      log::warn(R"(account="{}", origin={}, error={}, status={}, text="{}")"sv, account_.name, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::OpenPositionsAck open_positions_ack{body, decode_buffer_};
        Trace event_2{event, open_positions_ack};
        (*this)(event_2);
        download_.check(STATE);
      }
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<protocol::json::OpenPositionsAck> const &event) {
  auto &[trace_info, open_positions_ack] = event;
  log::info<4>("open_positions_ack={}"sv, open_positions_ack);
  assert(std::empty(open_positions_ack.error));
}

// open-orders

void OrderEntry::get_open_orders() {
  profile_.open_orders([&]() {
    auto method = web::http::Method::POST;
    auto path = shared_.api.order_management.open_orders;
    auto body = account_.create_body();
    auto headers = account_.create_headers(method, path, body);
    auto request = web::rest::Request{
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
    (*connection_)("open-orders"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_open_orders_ack(event, sequence);
    });
  });
}

void OrderEntry::get_open_orders_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  auto const STATE = State::OPEN_ORDERS;
  profile_.open_orders_ack([&]() {
    auto handle_error = [&](auto origin, auto status, auto error, auto text) {
      log::warn(R"(account="{}", origin={}, error={}, status={}, text="{}")"sv, account_.name, origin, error, status, text);
      download_.retry(STATE);
    };
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        protocol::json::OpenOrdersAck open_orders_ack{body, decode_buffer_};
        Trace event_2{event, open_orders_ack};
        (*this)(event_2);
        download_.check(STATE);
      }
    };
    process_response(event, handle_success, handle_error);
  });
}

void OrderEntry::operator()(Trace<protocol::json::OpenOrdersAck> const &event) {
  auto &[trace_info, open_orders_ack] = event;
  log::info<4>("open_orders_ack={}"sv, open_orders_ack);
  assert(std::empty(open_orders_ack.error));
  // note! we can't use because symbols are different
  /*
  for (auto &item : open_orders_ack.result.open) {
    auto order_update = server::oms::OrderUpdate{
        .account = account_.name,
        .exchange = shared_.settings.exchange,
        .symbol = item.descr.pair,
        .side = map(item.descr.type),
        .position_effect = {},
        .margin_mode = {},
        .max_show_quantity = NaN,
        .order_type = map(item.descr.ordertype),
        .time_in_force = map(item.time_in_force),
        .execution_instructions = {},
        .create_time_utc = item.opentm,
        .update_time_utc = {},
        .external_account = {},
        .external_order_id = item.KEY,
        .client_order_id = item.cl_ord_id,
        .order_status = OrderStatus::WORKING,
        .quantity = item.vol,
        .price = item.price,
        .stop_price = NaN,
        .leverage = NaN,
        .remaining_quantity = NaN,
        .traded_quantity = NaN,  // XXX
        .average_traded_price = {},
        .last_traded_quantity = {},
        .last_traded_price = {},
        .last_liquidity = {},
        .routing_id = {},
        .max_request_version = {},
        .max_response_version = {},
        .max_accepted_version = {},
        .update_type = UpdateType::SNAPSHOT,
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(shared_.dispatcher, trace_info, order_update, stream_id_);
  }
  */
}

// helpers

template <typename SuccessHandler, typename ErrorHandler>
void OrderEntry::process_response(web::rest::Response const &response, SuccessHandler success_handler, ErrorHandler error_handler) {
  try {
    auto [status, category, body] = response.result();
    switch (category) {
      using enum web::http::Category;
      case SUCCESS:  // 2xx
        success_handler(body);
        break;
      case CLIENT_ERROR: {  // 4xx
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, text);
        break;
      }
      case SERVER_ERROR: {  // 5xx
        auto text = fmt::format("{}"sv, status);
        error_handler(Origin::EXCHANGE, RequestStatus::REJECTED, Error::UNKNOWN, text);
        break;
      }
      default:
        response.expect(web::http::Status::OK);  // throws
    }
  } catch (server::oms::Exception &e) {
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

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

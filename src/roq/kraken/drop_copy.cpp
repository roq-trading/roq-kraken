/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/drop_copy.hpp"

#include "roq/mask.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/utils/charconv/to_string.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/server/oms/exceptions.hpp"

#include "roq/kraken/json/encoder.hpp"
#include "roq/kraken/json/map.hpp"
#include "roq/kraken/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === CONSTANTS ===

namespace {
auto const NAME = "ex"sv;

auto const SUPPORTS = Mask{
    SupportType::CREATE_ORDER,
    SupportType::MODIFY_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
    SupportType::ORDER,
    SupportType::FUNDS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;
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
      connection_{create_connection(*this, shared.settings, context)}, decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
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
      account_{account}, shared_{shared} {
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

void DropCopy::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

uint16_t DropCopy::operator()(
    Event<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  auto message = json::Encoder::add_order_json(encode_buffer_, event, order, ref_data, request_id, token_);
  log::warn("DEBUG {}"sv, message);
  (*connection_).send_text(message);
  return stream_id_;
}

uint16_t DropCopy::operator()(
    Event<ModifyOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  auto message = json::Encoder::amend_order_json(encode_buffer_, event, order, ref_data, request_id, previous_request_id, token_);
  log::warn("DEBUG {}"sv, message);
  (*connection_).send_text(message);
  return stream_id_;
}

uint16_t DropCopy::operator()(
    Event<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  auto message = json::Encoder::cancel_order_json(encode_buffer_, event, order, ref_data, request_id, previous_request_id, token_);
  log::warn("DEBUG {}"sv, message);
  (*connection_).send_text(message);
  return stream_id_;
}

uint16_t DropCopy::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  auto message = json::Encoder::cancel_all_json(encode_buffer_, event, request_id, token_);
  log::warn("DEBUG {}"sv, message);
  (*connection_).send_text(message);
  return stream_id_;
}

void DropCopy::subscribe() {
  subscribe("executions"sv);
  subscribe("balances"sv);
}

void DropCopy::subscribe(std::string_view const &channel) {
  log::info(R"(subscribe channel="{}", token="{}")"sv, channel, token_);
  assert(!std::empty(token_));
  std::string message;
  fmt::format_to(
      std::back_inserter(message),
      R"({{)"
      R"("method":"subscribe",)"
      R"("params":{{)"
      R"("channel":"{}",)"
      R"("token":"{}")"sv,
      channel,
      token_);
  if (channel == "executions"sv) {
    fmt::format_to(
        std::back_inserter(message),
        R"(,"snap_orders":true)"
        R"(,"snap_trades":true)"sv);
  }
  fmt::format_to(
      std::back_inserter(message),
      R"(}})"
      R"(}})"sv);
  log::info<3>(R"(request="{}")"sv, message);
  log::warn(R"(DEBUG request="{}")"sv, message);
  (*connection_).send_text(message);
}

// web::socket::Client::Handler

void DropCopy::operator()(web::socket::Client::Connected const &) {
  // note! wait for upgrade
}

void DropCopy::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
}

void DropCopy::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  // wait for status
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

void DropCopy::operator()(ConnectionStatus connection_status, std::string_view const &reason) {
  connection_status_ = connection_status;
  TraceInfo trace_info;
  auto stream_status = StreamStatus{
      .stream_id = stream_id_,
      .account = account_.name,
      .supports = SUPPORTS,
      .transport = Transport::TCP,
      .protocol = Protocol::WS,
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
  create_trace_and_dispatch(handler_, trace_info, stream_status);
}

void DropCopy::parse(std::string_view const &message) {
  profile_.parse([&]() {
    // log::warn("DEBUG message={}"sv, message);
    auto log_message = [&]() { log::warn(R"(*** PLEASE REPORT *** message="{}")"sv, message); };
    TraceInfo trace_info;
    try {
      if (!json::Parser::dispatch(*this, message, decode_buffer_, trace_info, shared_.settings.experimental.allow_unknown_event_types)) {
        log_message();
      }
    } catch (...) {
      log_message();
      utils::exceptions::Unhandled::terminate();
    }
  });
}

// json::Parser::Handler

void DropCopy::operator()(Trace<json::Status> const &event) {
  auto &[trace_info, status] = event;
  log::info("status={}"sv, status);
  (*this)(ConnectionStatus::READY);
  subscribe();  // note!
}

void DropCopy::operator()(Trace<json::Heartbeat> const &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<5>("heartbeat={}"sv, heartbeat);
  (*connection_).touch(trace_info.source_receive_time);
}

void DropCopy::operator()(Trace<json::Error> const &event) {
  auto &[trace_info, error] = event;
  log::error("error={}"sv, error);
}

void DropCopy::operator()(Trace<json::Pong> const &event) {
  auto &[trace_info, pong] = event;
  log::info<5>("pong={}"sv, pong);
  auto external_latency = trace_info.origin_create_time - std::chrono::nanoseconds{pong.req_id};
  log::info<5>("external_latency={}"sv, external_latency);
  (*connection_).touch(trace_info.source_receive_time);
}

void DropCopy::operator()(Trace<json::Subscribe> const &event) {
  auto &[trace_info, subscribe] = event;
  if (subscribe.success) {
    log::info<5>("subscribe={}"sv, subscribe);
  } else {
    log::error("subscribe={}"sv, subscribe);
  }
}

void DropCopy::operator()(Trace<json::Instrument> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Ticker> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Trade> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Book> const &) {
  log::fatal("Unexpected"sv);
}

void DropCopy::operator()(Trace<json::Balances> const &event) {
  auto &[trace_info, balances] = event;
  log::warn("DEBUG balances={}"sv, balances);
  for (auto &item : balances.data) {
    auto funds_update = FundsUpdate{
        .stream_id = stream_id_,
        .account = account_.name,
        .currency = item.asset,
        .margin_mode = {},
        .balance = item.amount,
        .hold = NaN,
        .borrowed = NaN,
        .external_account = {},
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = item.timestamp,
        .exchange_sequence = balances.sequence,
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, funds_update, true);
  }
}

void DropCopy::operator()(Trace<json::Executions> const &event) {
  auto &[trace_info, executions] = event;
  log::warn("DEBUG executions={}"sv, executions);
  auto update_type = map(executions.type).get<UpdateType>();
  for (auto &item : executions.data) {
    auto discard = [&]() {
      switch (item.exec_type) {
        using enum json::ExecType::type_t;
        case UNDEFINED_INTERNAL:
        case UNKNOWN_INTERNAL:
          break;
        case PENDING_NEW:
        case NEW:
          return false;
        case TRADE:
        case FILLED:
        case CANCELED:
        case ICEBERG_REFILL:
        case EXPIRED:
        case AMENDED:
        case RESTATED:
        case STATUS:  // ???
          break;
      }
      return executions.type == json::Type::SNAPSHOT;
    }();
    if (discard) {
      continue;  // note!
    }
    auto side = map(item.side).get<Side>();
    auto traded_quantity = [&]() {
      if (std::isnan(item.cum_qty)) {
        return 0.0;
      }
      return item.cum_qty;
    }();
    auto remaining_quantity = item.order_qty - traded_quantity;
    auto last_liquidity = map(item.liquidity_ind).get<Liquidity>();
    auto order_update = server::oms::OrderUpdate{
        .account = account_.name,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .side = side,
        .position_effect = {},
        .margin_mode = {},
        .max_show_quantity = NaN,
        .order_type = map(item.order_type),
        .time_in_force = map(item.time_in_force),
        .execution_instructions = {},
        .create_time_utc = item.timestamp,
        .update_time_utc = item.timestamp,
        .external_account = {},
        .external_order_id = item.order_id,
        .client_order_id = item.cl_ord_id,
        .order_status = map(item.order_status),
        .error = {},
        .text = {},
        .quantity = item.order_qty,
        .price = item.limit_price,
        .stop_price = NaN,
        .leverage = NaN,
        .remaining_quantity = remaining_quantity,
        .traded_quantity = traded_quantity,
        .average_traded_price = item.avg_price,
        .last_traded_quantity = item.last_qty,
        .last_traded_price = item.last_price,
        .last_liquidity = last_liquidity,
        .routing_id = {},
        .max_request_version = {},
        .max_response_version = {},
        .max_accepted_version = {},
        .update_type = update_type,
        .sending_time_utc = {},
    };
    log::warn("DEBUG order_update={}"sv, order_update);
    auto user_id = SOURCE_NONE;
    auto order_id = ORDER_ID_NONE;
    auto strategy_id = STRATEGY_ID_NONE;
    if (shared_.update_order(item.cl_ord_id, stream_id_, trace_info, order_update, [&](auto &order) {
          user_id = order.user_id;
          order_id = order.order_id;
          strategy_id = order.strategy_id;
        })) {
    } else {
      log::warn("*** EXTERNAL ORDER ***"sv);
    }
    if (item.exec_type == json::ExecType::TRADE && user_id != SOURCE_NONE) {
      auto fill = Fill{
          .exchange_time_utc = item.timestamp,
          .external_trade_id = {},
          .quantity = item.last_qty,
          .price = item.last_price,
          .liquidity = last_liquidity,
          .commission_amount = NaN,  // note! we only have it per TRADE
          .commission_currency = {},
          .base_amount = NaN,
          .quote_amount = NaN,
          .profit_loss_amount = NaN,
      };
      utils::charconv::to_string(std::back_inserter(fill.external_trade_id), item.trade_id);
      auto trade_update = TradeUpdate{
          .stream_id = stream_id_,
          .account = account_.name,
          .order_id = order_id,
          .exchange = shared_.settings.exchange,
          .symbol = item.symbol,
          .side = side,
          .position_effect = {},
          .margin_mode = {},
          .create_time_utc = item.timestamp,
          .update_time_utc = item.timestamp,
          .external_account = {},
          .external_order_id = item.order_id,
          .client_order_id = {},
          .fills = {&fill, 1},
          .routing_id = {},
          .update_type = update_type,
          .sending_time_utc = {},
          .user = {},
          .strategy_id = strategy_id,
      };
      create_trace_and_dispatch(handler_, trace_info, trade_update, true, user_id, item.cl_ord_id);
    }
    /*
    switch (item.exec_type) {
      using json::ExecType::type_t;
      case UNDEFINED_INTERNAL:
        break;
      case UNKNOWN_INTERNAL:
        break;
      case PENDING_NEW:
      case NEW:
      case TRADE:
      case FILLED:
      case CANCELED:
      case ICEBERG_REFILL:
        break;
      case EXPIRED:
        break;
      case AMENDED:
      case RESTATED:
        break;
      case STATUS:  // ???
        break;
    }
    */
  }
}

void DropCopy::operator()(Trace<json::AddOrder> const &event) {
  auto &[trace_info, add_order] = event;
  log::warn("DEBUG add_order={}"sv, add_order);
  if (std::empty(add_order.error)) {
    // note! using executions
  } else {
    auto error = json::guess_error(add_order.error);
    auto response = server::oms::Response{
        .request_type = RequestType::CREATE_ORDER,
        .origin = Origin::EXCHANGE,
        .request_status = RequestStatus::REJECTED,
        .error = error,
        .text = add_order.error,
        .version = 1,  // note!
        .request_id = {},
        .external_order_id = {},
        .quantity = NaN,
        .price = NaN,
    };
    auto [user_id, order_id] = json::Encoder::unpack_req_id(add_order.req_id);
    shared_.update_order(user_id, order_id, stream_id_, trace_info, response, []([[maybe_unused]] auto &order) {});
  }
}

void DropCopy::operator()(Trace<json::AmendOrder> const &event) {
  auto &[trace_info, amend_order] = event;
  log::warn("DEBUG amend_order={}"sv, amend_order);
  if (std::empty(amend_order.error)) {
    // note! using executions
  } else {
    auto error = json::guess_error(amend_order.error);
    auto response = server::oms::Response{
        .request_type = RequestType::MODIFY_ORDER,
        .origin = Origin::EXCHANGE,
        .request_status = RequestStatus::REJECTED,
        .error = error,
        .text = amend_order.error,
        .version = {},  // note! we don't have space in req_id => use heuristics
        .request_id = {},
        .external_order_id = {},
        .quantity = NaN,
        .price = NaN,
    };
    auto [user_id, order_id] = json::Encoder::unpack_req_id(amend_order.req_id);
    shared_.update_order(user_id, order_id, stream_id_, trace_info, response, []([[maybe_unused]] auto &order) {});
  }
}

void DropCopy::operator()(Trace<json::CancelOrder> const &event) {
  auto &[trace_info, cancel_order] = event;
  log::warn("DEBUG cancel_order={}"sv, cancel_order);
  if (std::empty(cancel_order.error)) {
    // note! using executions
  } else {
    auto error = json::guess_error(cancel_order.error);
    auto response = server::oms::Response{
        .request_type = RequestType::CANCEL_ORDER,
        .origin = Origin::EXCHANGE,
        .request_status = RequestStatus::REJECTED,
        .error = error,
        .text = cancel_order.error,
        .version = {},  // note! we don't have space in req_id => use heuristics
        .request_id = {},
        .external_order_id = {},
        .quantity = NaN,
        .price = NaN,
    };
    auto [user_id, order_id] = json::Encoder::unpack_req_id(cancel_order.req_id);
    shared_.update_order(user_id, order_id, stream_id_, trace_info, response, []([[maybe_unused]] auto &order) {});
  }
}

void DropCopy::operator()(Trace<json::CancelAll> const &event) {
  auto &[trace_info, cancel_all] = event;
  log::warn("DEBUG cancel_all={}"sv, cancel_all);
  // XXX FIXME TODO ack
}

}  // namespace kraken
}  // namespace roq

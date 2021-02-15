/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/gateway.h"

#include <limits>
#include <utility>

#include "roq/core/update.h"
#include "roq/core/utils.h"

#include "roq/kraken/flags.h"

#include "roq/kraken/json/utils.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

template <typename C, typename T>
static bool mbp_update(C &data, size_t &offset, const T &item) {
  if (offset >= data.size())
    return false;
  auto &obj = data[offset];
  new (&obj) MBPUpdate{
      .price = item.price,
      .quantity = item.volume,
  };
  ++offset;
  return offset <= data.size();
}

template <typename C, typename T>
static bool trade_update(C &data, size_t &offset, const T &item) {
  if (offset >= data.size())
    return false;
  auto &obj = data[offset];
  new (&obj) Trade{
      .side = json::map(item.side),
      .price = item.price,
      .quantity = item.volume,
      .trade_id = {},
  };
  ++offset;
  return offset <= data.size();
}

Gateway::Gateway(server::Dispatcher &dispatcher, const Config &config)
    : dispatcher_(dispatcher), account_(config.get_account()), access_key_(config.get_access_key()),
      random_(config.get_access_key(), config.get_access_secret(), config.get_access_password()),
      dns_base_(base_, true),
      web_socket_public_{
          .connection =
              {
                  *this,
                  config,
                  random_,
                  base_,
                  dns_base_,
                  ssl_context_,
              },
          .download = WebSocketDownload(
              std::chrono::seconds{Flags::ws_public_request_timeout_secs()},
              [this](auto state) { return download(state); }),
      },
      web_socket_private_{
          .connection =
              {
                  *this,
                  config,
                  random_,
                  base_,
                  dns_base_,
                  ssl_context_,
              },
          .download = WebSocketPrivateDownload(
              std::chrono::seconds{Flags::ws_private_request_timeout_secs()},
              [this](auto state) { return download(state); }),
      },
      rest_{
          .connection =
              {
                  *this,
                  config,
                  random_,
                  base_,
                  dns_base_,
                  ssl_context_,
              },
      },
      bid_(Flags::cache_mbp_max_depth()), ask_(Flags::cache_mbp_max_depth()),
      trade_(Flags::cache_trades_max_depth()) {
}

void Gateway::operator()(const Event<Start> &event) {
  LOG(INFO)("Starting the gateway..."_sv);
  web_socket_public_.connection(event);
  web_socket_private_.connection(event);
  rest_.connection(event);
}

void Gateway::operator()(const Event<Stop> &event) {
  LOG(INFO)("Stopping the gateway..."_sv);
  rest_.connection(event);
  web_socket_private_.connection(event);
  web_socket_public_.connection(event);
}

void Gateway::operator()(const Event<Timer> &event) {
  web_socket_public_.connection(event);
  web_socket_private_.connection(event);
  rest_.connection(event);
  // download
  /*
  if (_web_socket.download.has_expired()) {
    LOG(WARNING)("WebSocket download has timed out"_sv);
    _web_socket.download.reset();
    _web_socket.connection.close();
  }
  */
  base_.loop(EVLOOP_NONBLOCK);
}

void Gateway::operator()(const Event<Connection> &) {
}

void Gateway::operator()(
    [[maybe_unused]] const Event<CreateOrder> &event,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] uint32_t gateway_order_id) {
  // TODO(thraneh): implement
}

void Gateway::operator()(
    [[maybe_unused]] const Event<ModifyOrder> &event,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const server::OMS_Order &order) {
  // TODO(thraneh): implement
}

void Gateway::operator()(
    [[maybe_unused]] const Event<CancelOrder> &event,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const server::OMS_Order &order) {
  // TODO(thraneh): implement
}

void Gateway::operator()(metrics::Writer &writer) {
  rest_.connection(writer);
  web_socket_public_.connection(writer);
  web_socket_private_.connection(writer);
}

// all

void Gateway::operator()(
    const ExternalLatency &external_latency, const server::TraceInfo &trace_info) {
  create_trace_and_dispatch(trace_info, external_latency, dispatcher_);
}

// rest

void Gateway::operator()(const Rest &) {
  if (rest_.connection.ready()) {
    web_socket_public_.download.bump();
    web_socket_private_.download.bump();
  }
}

void Gateway::download_assets() {
  constexpr auto state = WebSocketDownload::State::ASSETS;
  auto sequence = web_socket_public_.download.sequence();
  rest_.connection.get<json::Assets>([this, sequence](auto &promise) {
    try {
      if (web_socket_public_.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      web_socket_public_.download.check(state);
    } catch (NetworkError &) {
      web_socket_public_.download.retry(state);
    }
  });
}

void Gateway::download_asset_pairs() {
  constexpr auto state = WebSocketDownload::State::ASSET_PAIRS;
  auto sequence = web_socket_public_.download.sequence();
  rest_.connection.get<json::AssetPairs>([this, sequence](auto &promise) {
    try {
      if (web_socket_public_.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      web_socket_public_.download.check(state);
    } catch (NetworkError &) {
      web_socket_public_.download.retry(state);
    }
  });
}

void Gateway::download_balance() {
  constexpr auto state = WebSocketDownload::State::BALANCE;
  std::ignore = state;
  /*
  auto sequence = web_socket_public_.download.sequence();
  rest_.connection.get<json::Balance>(
      [this, sequence](auto& promise) {
    try {
      if (web_socket_public_.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      web_socket_public_.download.check(state);
    } catch (NetworkError&) {
      web_socket_public_.download.retry(state);
    }
  });
  */
}

void Gateway::download_open_positions() {
  constexpr auto state = WebSocketDownload::State::OPEN_POSITIONS;
  auto sequence = web_socket_public_.download.sequence();
  rest_.connection.get<json::Positions>([this, sequence](auto &promise) {
    try {
      if (web_socket_public_.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      web_socket_public_.download.check(state);
    } catch (NetworkError &) {
      web_socket_public_.download.retry(state);
    }
  });
}

void Gateway::operator()(const json::Assets &) {
}

void Gateway::operator()(const json::AssetPairs &asset_pairs) {
  assert(asset_pairs.error.empty());
  assert(symbols_.empty());
  server::TraceInfo trace_info;  // XXX not correct (*parsing* already done)
  symbols_.reserve(asset_pairs.result.size());
  for (auto &item : asset_pairs.result) {
    VLOG(1)(R"(item={})"_sv, item);
    if (item.wsname.empty()) {
      VLOG(1)(R"(Skipping altname={}, reason: wsname is empty)"_sv, item.altname);
      continue;
    }
    std::string symbol(item.wsname);
    // XXX remove escape
    symbol.erase(std::remove(symbol.begin(), symbol.end(), '\\'), symbol.end());
    if (dispatcher_.discard_symbol(symbol))
      continue;
    symbols_.emplace_back(symbol);
    auto tick_size = std::pow(double{10.0}, -static_cast<double>(item.pair_decimals));
    auto min_trade_vol = std::pow(double{10.0}, -static_cast<double>(item.lot_decimals));
    ReferenceData reference_data{
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .description = item.altname,
        .security_type = SecurityType::UNDEFINED,
        .currency = item.aclass_quote,
        .settlement_currency = item.aclass_base,
        .commission_currency = item.aclass_base,
        .tick_size = tick_size,
        .multiplier = item.lot_multiplier,  // XXX check
        .min_trade_vol = min_trade_vol,
        .option_type = OptionType::UNDEFINED,
        .strike_currency = {},
        .strike_price = std::numeric_limits<double>::quiet_NaN(),
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
    };
    VLOG(1)(R"(reference_data={})"_sv, reference_data);
    server::create_trace_and_dispatch(trace_info, reference_data, dispatcher_, true);
    MarketStatus market_status{
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .trading_status = TradingStatus::OPEN,  // XXX doesn't exist?
    };
    VLOG(2)(R"(market_status={})"_sv, market_status);
    server::create_trace_and_dispatch(trace_info, market_status, dispatcher_, true);
  }
}

void Gateway::operator()(const json::Positions &positions) {
  assert(positions.error.empty());
}

void Gateway::operator()(const json::Token &token) {
  LOG(INFO)(R"(token={})"_sv, token);
  // XXX maybe we have to URL decode here ???
  token_ = token.token;
}

// web socket

int32_t Gateway::download(WebSocketDownload::State state) {
  if (web_socket_public_.connection.ready() == false)
    return -1;
  switch (state) {
    case WebSocketDownload::State::UNDEFINED:
      assert(false);
      break;
    case WebSocketDownload::State::ASSETS:
      download_assets();
      return 1;
    case WebSocketDownload::State::ASSET_PAIRS:
      download_asset_pairs();
      return 1;
    case WebSocketDownload::State::BALANCE:
      download_balance();
      // return 1;
      return 0;
    case WebSocketDownload::State::OPEN_POSITIONS:
      download_open_positions();
      return 1;
    case WebSocketDownload::State::SUBSCRIBE_PUBLIC:
      subscribe_public();
      return 0;
    case WebSocketDownload::State::DONE:
      update(GatewayStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

void Gateway::operator()(const WebSocketPublic &) {
  if (web_socket_public_.connection.ready()) {
    web_socket_public_.download.begin();
  } else {
    web_socket_public_.download.reset();
    symbols_.clear();
  }
}

void Gateway::subscribe_public() {
  roq::span pairs(symbols_.data(), symbols_.size());
  web_socket_public_.connection.subscribe("trade"_sv, pairs);
  web_socket_public_.connection.subscribe("spread"_sv, pairs);
  web_socket_public_.connection.subscribe("book"_sv, pairs);
}

void Gateway::operator()(
    const json::Trade &trade, const std::string_view &pair, const server::TraceInfo &trace_info) {
  bool success = true;
  std::chrono::nanoseconds exchange_time_utc = {};
  size_t trade_length = 0;
  for (auto &item : trade.data) {
    if (success == false)
      break;
    success = trade_update(trade_, trade_length, item);
    core::update_first(exchange_time_utc, item.time);
  }
  LOG_IF(WARNING, !success)
  (R"(Insufficient trade array size: )"
   R"(symbol="{}", len(trade)={}/{})"_sv,
   pair,
   trade.data.size(),
   trade_.size());
  if (trade_length > 0) {
    TradeSummary trade_summary{
        .exchange = Flags::exchange(),
        .symbol = pair,
        .trades = {trade_.data(), trade_length},
        .exchange_time_utc = exchange_time_utc,
    };
    VLOG(3)(R"(trade_summary={})"_sv, trade_summary);
    server::create_trace_and_dispatch(trace_info, trade_summary, dispatcher_, true);
  }
}

void Gateway::operator()(
    const json::Spread &spread, const std::string_view &pair, const server::TraceInfo &trace_info) {
  TopOfBook top_of_book{
      .exchange = Flags::exchange(),
      .symbol = pair,
      .layer =
          {
              .bid_price = spread.bid,
              .bid_quantity = spread.bid_volume,
              .ask_price = spread.ask,
              .ask_quantity = spread.ask_volume,
          },
      .snapshot = false,  // note! we don't know... false is probably ok
      .exchange_time_utc = spread.timestamp,
  };
  VLOG(3)(R"(top_of_book={})"_sv, top_of_book);
  server::create_trace_and_dispatch(trace_info, top_of_book, dispatcher_, true);
}

void Gateway::operator()(
    const json::Book &book, const std::string_view &pair, const server::TraceInfo &trace_info) {
  bool snapshot = book.bs.empty() == false && book.as.empty() == false;
  bool live = book.b.empty() == false && book.a.empty() == false;
  LOG_IF(FATAL, snapshot && live)("Unexpected"_sv);
  bool success = true;
  std::chrono::nanoseconds exchange_time_utc = {};
  size_t bid_length = 0, ask_length = 0;
  for (auto &item : book.b) {
    if (success == false)
      break;
    success = mbp_update(bid_, bid_length, item);
    core::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.bs) {
    if (success == false)
      break;
    success = mbp_update(bid_, bid_length, item);
    core::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.a) {
    if (success == false)
      break;
    success = mbp_update(ask_, ask_length, item);
    core::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.as) {
    if (success == false)
      break;
    success = mbp_update(ask_, ask_length, item);
    core::update_first(exchange_time_utc, item.timestamp);
  }
  LOG_IF(WARNING, !success)
  (R"(Insufficient bid/ask array size(s): )"
   R"(symbol="{}", len(bid)={}+{}/{}, len(ask)={}+{}/{})"_sv,
   pair,
   book.b.size(),
   book.bs.size(),
   bid_.size(),
   book.a.size(),
   book.as.size(),
   ask_.size());
  if (bid_length > 0 || ask_length > 0) {
    MarketByPriceUpdate market_by_price_update{
        .exchange = Flags::exchange(),
        .symbol = pair,
        .bids = {bid_.data(), bid_length},
        .asks = {ask_.data(), ask_length},
        .snapshot = snapshot,
        .exchange_time_utc = exchange_time_utc,
    };
    VLOG(3)(R"(market_by_price_update={})"_sv, market_by_price_update);
    server::create_trace_and_dispatch(trace_info, market_by_price_update, dispatcher_, true);
  }
}

// web-socket (private)

int32_t Gateway::download(WebSocketPrivateDownload::State state) {
  if (web_socket_private_.connection.ready() == false)
    return -1;
  switch (state) {
    case WebSocketPrivateDownload::State::UNDEFINED:
      assert(false);
      break;
    case WebSocketPrivateDownload::State::WEB_SOCKETS_TOKEN:
      download_web_sockets_token();
      return 1;
    case WebSocketPrivateDownload::State::SUBSCRIBE_PRIVATE:
      subscribe_private();
      return 0;
    case WebSocketPrivateDownload::State::DONE:
      update(GatewayStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

void Gateway::operator()(const WebSocketPrivate &) {
  if (web_socket_private_.connection.ready()) {
    web_socket_private_.download.begin();
  } else {
    web_socket_private_.download.reset();
    token_.clear();
  }
}

void Gateway::download_web_sockets_token() {
  constexpr auto state = WebSocketPrivateDownload::State::WEB_SOCKETS_TOKEN;
  auto sequence = web_socket_private_.download.sequence();
  rest_.connection.get<json::Token>([this, sequence](auto &promise) {
    try {
      if (web_socket_private_.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      web_socket_private_.download.check(state);
    } catch (NetworkError &) {
      web_socket_private_.download.retry(state);
    }
  });
}

void Gateway::subscribe_private() {
  web_socket_private_.connection.subscribe("ownTrades"_sv, token_);
  web_socket_private_.connection.subscribe("openOrders"_sv, token_);
}

void Gateway::operator()(const json::AddOrderStatus &, const server::TraceInfo &) {
}

void Gateway::operator()(const json::CancelOrderStatus &, const server::TraceInfo &) {
}

void Gateway::operator()(const json::OpenOrders &, const server::TraceInfo &) {
}

void Gateway::operator()(const json::OwnTrades &, const server::TraceInfo &) {
}

void Gateway::update(GatewayStatus gateway_status) {
  if (gateway_status == gateway_status_)
    return;
  gateway_status_ = gateway_status;
  server::TraceInfo trace_info;
  MarketDataStatus market_data_status{
      .status = gateway_status_,
  };
  server::create_trace_and_dispatch(trace_info, market_data_status, dispatcher_, false);
  OrderManagerStatus order_manager_status{
      .account = account_,
      .status = gateway_status_,
  };
  server::create_trace_and_dispatch(trace_info, order_manager_status, dispatcher_, true);
  LOG(INFO)(R"(Update: gateway_status={})"_sv, gateway_status_);
}

}  // namespace kraken
}  // namespace roq

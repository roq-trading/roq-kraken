/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/gateway.h"

#include <limits>
#include <utility>

#include "roq/core/utils.h"

#include "roq/kraken/options.h"

#include "roq/kraken/json/utils.h"

namespace roq {
namespace kraken {

template <typename T>
static bool mbp_update(
    auto& data,
    size_t& offset,
    const T& item) {
  auto& obj = data[offset];
  new (&obj) MBPUpdate {
    .price = item.price,
    .quantity = item.volume,
  };
  ++offset;
  return offset < data.size();
}

template <typename T>
static bool trade_update(
    auto& data,
    size_t& offset,
    const T& item) {
  auto& obj = data[offset];
  new (&obj) Trade {
    .side = json::map(item.side),
    .price = item.price,
    .quantity = item.volume,
    .trade_id = {},
  };
  ++offset;
  return offset < data.size();
}

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const Config& config)
    : _dispatcher(dispatcher),
      _account(config.get_account()),
      _access_key(config.get_access_key()),
      _random(
          config.get_access_key(),
          config.get_access_secret(),
          config.get_access_password()),
      _dns_base(_base, true),
      _web_socket_public {
        .connection = {
          *this,
          config,
          _random,
          _base,
          _dns_base,
          _ssl_context,
        },
        .download = WebSocketDownload(
            std::chrono::seconds { FLAGS_download_timeout_secs },
            [this](auto state) {
              return download(state);
            }),
      },
      _web_socket_private {
        .connection = {
          *this,
          config,
          _random,
          _base,
          _dns_base,
          _ssl_context,
        },
        .download = WebSocketPrivateDownload(
            std::chrono::seconds { FLAGS_download_timeout_secs },
            [this](auto state) {
              return download(state);
            }),
      },
      _rest {
        .connection = {
          *this,
          config,
          _random,
          _base,
          _dns_base,
          _ssl_context,
        },
      },
      _bid(FLAGS_cache_mbp_max_depth),
      _ask(FLAGS_cache_mbp_max_depth),
      _trade(FLAGS_cache_trades_max_depth) {
}

void Gateway::operator()(const server::StartEvent& event) {
  LOG(INFO)("Starting the gateway...");
  _web_socket_public.connection(event);
  _web_socket_private.connection(event);
  _rest.connection(event);
}

void Gateway::operator()(const server::StopEvent& event) {
  LOG(INFO)("Stopping the gateway...");
  _rest.connection(event);
  _web_socket_private.connection(event);
  _web_socket_public.connection(event);
}

void Gateway::operator()(const server::TimerEvent& event) {
  _web_socket_public.connection(event);
  _web_socket_private.connection(event);
  _rest.connection(event);
  // download
  /*
  if (_web_socket.download.has_expired()) {
    LOG(WARNING)("WebSocket download has timed out");
    _web_socket.download.reset();
    _web_socket.connection.close();
  }
  */
  _base.loop(EVLOOP_NONBLOCK);
}

void Gateway::operator()(const server::ConnectionStatusEvent&) {
}

void Gateway::operator()(
    const CreateOrderEvent& event,
    const std::string_view& request_id,
    uint32_t gateway_order_id) {
  // TODO(thraneh): implement
  (void)(event);
  (void)(request_id);
  (void)(gateway_order_id);
}

void Gateway::operator()(
    const ModifyOrderEvent& event,
    const std::string_view& request_id,
    const server::OMS_Order& order) {
  // TODO(thraneh): implement
  (void)(event);
  (void)(request_id);
  (void)(order);
}

void Gateway::operator()(
    const CancelOrderEvent& event,
    const std::string_view& request_id,
    const server::OMS_Order& order) {
  // TODO(thraneh): implement
  (void)(event);
  (void)(request_id);
  (void)(order);
}

void Gateway::operator()(metrics::Writer& writer) {
  _rest.connection(writer);
  _web_socket_public.connection(writer);
  _web_socket_private.connection(writer);
}

// rest

void Gateway::operator()(const Rest&) {
  if (_rest.connection.ready()) {
    _web_socket_public.download.bump();
    _web_socket_private.download.bump();
  }
}

void Gateway::download_assets() {
  constexpr auto state = WebSocketDownload::State::ASSETS;
  auto sequence = _web_socket_public.download.sequence();
  _rest.connection.get<json::Assets>(
      [this, sequence](auto& promise) {
    try {
      if (_web_socket_public.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      _web_socket_public.download.check(state);
    } catch (NetworkError&) {
      _web_socket_public.download.retry(state);
    }
  });
}

void Gateway::download_asset_pairs() {
  constexpr auto state = WebSocketDownload::State::ASSET_PAIRS;
  auto sequence = _web_socket_public.download.sequence();
  _rest.connection.get<json::AssetPairs>(
      [this, sequence](auto& promise) {
    try {
      if (_web_socket_public.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      _web_socket_public.download.check(state);
    } catch (NetworkError&) {
      _web_socket_public.download.retry(state);
    }
  });
}

void Gateway::download_balance() {
  constexpr auto state = WebSocketDownload::State::BALANCE;
  (void)(state);
  /*
  auto sequence = _web_socket_public.download.sequence();
  _rest.connection.get<json::Balance>(
      [this, sequence](auto& promise) {
    try {
      if (_web_socket_public.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      _web_socket_public.download.check(state);
    } catch (NetworkError&) {
      _web_socket_public.download.retry(state);
    }
  });
  */
}

void Gateway::download_open_positions() {
  constexpr auto state = WebSocketDownload::State::OPEN_POSITIONS;
  auto sequence = _web_socket_public.download.sequence();
  _rest.connection.get<json::Positions>(
      [this, sequence](auto& promise) {
    try {
      if (_web_socket_public.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      _web_socket_public.download.check(state);
    } catch (NetworkError&) {
      _web_socket_public.download.retry(state);
    }
  });
}

void Gateway::operator()(const json::Assets&) {
}

void Gateway::operator()(const json::AssetPairs& asset_pairs) {
  assert(asset_pairs.error.empty());
  assert(_symbols.empty());
  server::Trace trace;  // XXX not correct (*parsing* already done)
  _symbols.reserve(asset_pairs.result.size());
  for (auto& item : asset_pairs.result) {
    if (item.wsname.empty()) {
      VLOG(1)(
          FMT_STRING(R"(Skipping altname={}, reason: wsname is empty)"),
          item.altname);
      continue;
    }
    std::string symbol(item.wsname);
    // XXX remove escape
    symbol.erase(
        std::remove(
            symbol.begin(),
            symbol.end(),
            '\\'),
        symbol.end());
    if (_dispatcher.discard_symbol(symbol))
      continue;
    _symbols.emplace_back(symbol);
    ReferenceData reference_data {
      .exchange = FLAGS_exchange,
      .symbol = symbol,
      .security_type = SecurityType::UNDEFINED,
      .currency = item.aclass_quote,  // XXX check
      .settlement_currency = item.aclass_base,  // XXX check
      .commission_currency = item.aclass_base,  // XXX check
      .tick_size = std::pow(10.0, -item.pair_decimals),  // XXX check
      .limit_up = std::numeric_limits<double>::quiet_NaN(),
      .limit_down = std::numeric_limits<double>::quiet_NaN(),
      .multiplier = item.lot_multiplier,  // XXX check
      .min_trade_vol = std::pow(10.0, -item.lot_decimals),  // XXX check
      .option_type = OptionType::UNDEFINED,
      .strike_currency = std::string_view(),
      .strike_price = std::numeric_limits<double>::quiet_NaN(),
    };
    VLOG(1)(
        FMT_STRING(R"(reference_data={})"),
        reference_data);
    enqueue(
        reference_data,
        trace,
        true);
    MarketStatus market_status {
      .exchange = FLAGS_exchange,
      .symbol = symbol,
      .trading_status = TradingStatus::OPEN,  // XXX doesn't exist?
    };
    VLOG(2)(
        FMT_STRING(R"(market_status={})"),
        market_status);
    enqueue(
        market_status,
        trace,
        true);
  }
}

void Gateway::operator()(const json::Positions& positions) {
  assert(positions.error.empty());
}

void Gateway::operator()(const json::Token& token) {
  LOG(INFO)(
      FMT_STRING(R"(token={})"),
      token);
  // XXX maybe we have to URL decode here ???
  _token = token.token;
}

// web socket

int32_t Gateway::download(WebSocketDownload::State state) {
  if (_web_socket_public.connection.ready() == false)
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

void Gateway::operator()(const WebSocketPublic&) {
  if (_web_socket_public.connection.ready()) {
    _web_socket_public.download.begin();
  } else {
    _web_socket_public.download.reset();
    _symbols.clear();
  }
}

void Gateway::subscribe_public() {
  roq::span pairs(
      _symbols.data(),
      _symbols.size());
  _web_socket_public.connection.subscribe(
      "trade",
      pairs);
  _web_socket_public.connection.subscribe(
      "spread",
      pairs);
  _web_socket_public.connection.subscribe(
      "book",
      pairs);
}

void Gateway::operator()(
    const json::Trade& trade,
    const std::string_view& pair,
    const server::Trace& trace) {
  bool success = true;
  std::chrono::nanoseconds exchange_time_utc = {};
  size_t trade_length = 0;
  for (auto& item : trade.data) {
    if (success == false)
      break;
    success = trade_update(
        _trade,
        trade_length,
        item);
    if (exchange_time_utc.count() == 0)
      exchange_time_utc = item.time;
  }
  if (ROQ_PREDICT_FALSE(success == false)) {
    LOG(FATAL)(
        FMT_STRING(
          R"(Insufficient trade array size: )"
          R"(len(trade)={}/{})"),
        trade_length, _trade.size());
  }
  if (trade_length > 0) {
    TradeSummary trade_summary {
      .exchange = FLAGS_exchange,
      .symbol = pair,
      .trades = {
        .items = _trade.data(),
        .length = trade_length,
      },
      .exchange_time_utc = exchange_time_utc,
    };
    VLOG(3)(
        FMT_STRING(R"(trade_summary={})"),
        trade_summary);
    enqueue(
        trade_summary,
        trace,
        true);
  }
}

void Gateway::operator()(
    const json::Spread& spread,
    const std::string_view& pair,
    const server::Trace& trace) {
  TopOfBook top_of_book {
    .exchange = FLAGS_exchange,
    .symbol = pair,
    .layer = {
      .bid_price = spread.bid,
      .bid_quantity = spread.bid_volume,
      .ask_price = spread.ask,
      .ask_quantity = spread.ask_volume,
    },
    .snapshot = false,  // note! we don't know... false is probably ok
    .exchange_time_utc = spread.timestamp,
  };
  VLOG(3)(
      FMT_STRING(R"(top_of_book={})"),
      top_of_book);
  enqueue(
      top_of_book,
      trace,
      true);
}

void Gateway::operator()(
    const json::Book& book,
    const std::string_view& pair,
    const server::Trace& trace) {
  bool snapshot =
    book.bs.empty() == false &&
    book.as.empty() == false;
  bool live =
    book.b.empty() == false &&
    book.a.empty() == false;
  LOG_IF(FATAL, snapshot && live)("Unexpected");
  bool success = true;
  std::chrono::nanoseconds exchange_time_utc = {};
  size_t bid_length = 0, ask_length = 0;
  for (auto& item : book.b) {
    if (success == false)
      break;
    success = mbp_update(
        _bid,
        bid_length,
        item);
    if (exchange_time_utc.count() == 0)
      exchange_time_utc = item.timestamp;
  }
  for (auto& item : book.bs) {
    if (success == false)
      break;
    success = mbp_update(
        _bid,
        bid_length,
        item);
    if (exchange_time_utc.count() == 0)
      exchange_time_utc = item.timestamp;
  }
  for (auto& item : book.a) {
    if (success == false)
      break;
    success = mbp_update(
        _ask,
        ask_length,
        item);
    if (exchange_time_utc.count() == 0)
      exchange_time_utc = item.timestamp;
  }
  for (auto& item : book.as) {
    if (success == false)
      break;
    success = mbp_update(
        _ask,
        ask_length,
        item);
    if (exchange_time_utc.count() == 0)
      exchange_time_utc = item.timestamp;
  }
  if (ROQ_PREDICT_FALSE(success == false)) {
    LOG(FATAL)(
        FMT_STRING(
          R"(Insufficient bid/ask array size(s): )"
          R"(len(bid={}/{}, len(ask)={}/{})"),
        bid_length, _bid.size(),
        ask_length, _ask.size());
  }
  if (bid_length > 0 || ask_length > 0) {
    MarketByPriceUpdate market_by_price_update {
      .exchange = FLAGS_exchange,
      .symbol = pair,
      .bids = {
        .items = _bid.data(),
        .length = bid_length,
      },
      .asks = {
        .items = _ask.data(),
        .length = ask_length,
      },
      .snapshot = snapshot,
      .exchange_time_utc = exchange_time_utc,
    };
    VLOG(3)(
        FMT_STRING(R"(market_by_price_update={})"),
        market_by_price_update);
    enqueue(
        market_by_price_update,
        trace,
        true);
  }
}

// web-socket (private)

int32_t Gateway::download(WebSocketPrivateDownload::State state) {
  if (_web_socket_private.connection.ready() == false)
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

void Gateway::operator()(const WebSocketPrivate&) {
  if (_web_socket_private.connection.ready()) {
    _web_socket_private.download.begin();
  } else {
    _web_socket_private.download.reset();
    _token.clear();
  }
}

void Gateway::download_web_sockets_token() {
  constexpr auto state = WebSocketPrivateDownload::State::WEB_SOCKETS_TOKEN;
  auto sequence = _web_socket_private.download.sequence();
  _rest.connection.get<json::Token>(
      [this, sequence](auto& promise) {
    try {
      if (_web_socket_private.download.skip(sequence, state))
        return;
      (*this)(promise.get());
      _web_socket_private.download.check(state);
    } catch (NetworkError&) {
      _web_socket_private.download.retry(state);
    }
  });
}

void Gateway::subscribe_private() {
  _web_socket_private.connection.subscribe(
      "ownTrades",
      _token);
  _web_socket_private.connection.subscribe(
      "openOrders",
      _token);
}

void Gateway::operator()(
    const json::AddOrderStatus&,
    const server::Trace&) {
}

void Gateway::operator()(
    const json::CancelOrderStatus&,
    const server::Trace&) {
}

void Gateway::operator()(
    const json::OpenOrders&,
    const server::Trace&) {
}

void Gateway::operator()(
    const json::OwnTrades&,
    const server::Trace&) {
}

void Gateway::update(GatewayStatus gateway_status) {
  if (gateway_status == _gateway_status)
    return;
  _gateway_status = gateway_status;
  server::Trace trace;
  MarketDataStatus market_data_status {
    .status = _gateway_status,
  };
  enqueue(
      market_data_status,
      trace,
      false);
  OrderManagerStatus order_manager_status {
    .account = _account,
    .status = _gateway_status,
  };
  enqueue(
      order_manager_status,
      trace,
      true);
  LOG(INFO)(
      FMT_STRING(R"(Update: gateway_status={})"),
      _gateway_status);
}

template <typename T>
inline void Gateway::enqueue(
    const T& value,
    const server::Trace& trace,
    bool is_last) {
  _dispatcher(
      value,
      trace,
      is_last);
}

template <typename T>
inline void Gateway::enqueue(
    uint8_t user_id,
    const T& value,
    const server::Trace& trace,
    bool is_last) {
  _dispatcher(
      user_id,
      value,
      trace,
      is_last);
}

}  // namespace kraken
}  // namespace roq

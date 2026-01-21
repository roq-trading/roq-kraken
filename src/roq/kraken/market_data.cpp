/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/market_data.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/exceptions/unhandled.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/utils/charconv/to_string.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/kraken/json/map.hpp"
#include "roq/kraken/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === CONSTANTS ===

namespace {
auto const NAME = "md"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
    SupportType::STATISTICS,
};

size_t const MAX_DECODE_BUFFER_DEPTH = 2;
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = settings.ws.public_uri;
  auto config = web::socket::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .host = {},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = settings.net.connection_timeout,
      .disconnect_on_idle_timeout = settings.net.disconnect_on_idle_timeout,
      .always_reconnect = true,
      // proxy
      .proxy = {},
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = {},
      .ping_frequency = settings.ws.public_ping_freq,
      // implementation
      .decode_buffer_size = settings.misc.decode_buffer_size,  // XXX need read buffer size
      .encode_buffer_size = settings.misc.encode_buffer_size,
  };
  return web::socket::Client::create(handler, context, config, []() { return std::string(); });
}

struct create_metrics final : public utils::metrics::Factory {
  create_metrics(auto &settings, auto &group, auto const &function) : utils::metrics::Factory{settings.app.name, group, function} {}
};
}  // namespace

// === IMPLEMENTATION ===

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)}, index_{index}, connection_{create_connection(*this, shared.settings, context)},
      decode_buffer_{shared.settings.misc.decode_buffer_size, MAX_DECODE_BUFFER_DEPTH},
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
      shared_{shared} {
}

void MarketData::operator()(Event<Start> const &) {
  (*connection_).start();
}

void MarketData::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void MarketData::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
  if (!ready()) {
    return;
  }
  auto now = event.value.now;
  if (next_heartbeat_ < now) {
    next_heartbeat_ = now + shared_.settings.ws.public_ping_freq;
    auto message = fmt::format(
        R"({{)"
        R"("method":"ping",)"
        R"("req_id":{})"
        R"(}})"sv,
        now.count());
    log::info<3>(R"(request="{}")"sv, message);
    (*connection_).send_text(message);
  }
}

void MarketData::operator()(metrics::Writer &writer) const {
  writer
      // counter
      .write(counter_.disconnect, metrics::Type::COUNTER)
      // profile
      .write(profile_.parse, metrics::Type::PROFILE)
      // latency
      .write(latency_.ping, metrics::Type::LATENCY)
      .write(latency_.heartbeat, metrics::Type::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready()) {
    subscribe(shared_.symbols.get_slice(index_, start_from));
  }
}

void MarketData::operator()(web::socket::Client::Connected const &) {
  // note! wait for upgrade
}

void MarketData::operator()(web::socket::Client::Disconnected const &) {
  ++counter_.disconnect;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
}

void MarketData::operator()(web::socket::Client::Ready const &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  // wait for status
}

void MarketData::operator()(web::socket::Client::Close const &) {
}

void MarketData::operator()(web::socket::Client::Latency const &latency) {
  TraceInfo trace_info;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(web::socket::Client::Text const &text) {
  parse(text.payload);
}

void MarketData::operator()(web::socket::Client::Binary const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
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

void MarketData::subscribe_static() {
  subscribe("instrument"sv);
}

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  subscribe("ticker"sv, symbols);
  subscribe("trade"sv, symbols);
  subscribe("book"sv, symbols);
}

void MarketData::subscribe(std::string_view const &channel) {
  auto message = fmt::format(
      R"({{)"
      R"("method":"subscribe",)"
      R"("params":{{)"
      R"("channel":"{}")"
      R"(}})"
      R"(}})"sv,
      channel);
  log::info<3>(R"(request="{}")"sv, message);
  (*connection_).send_text(message);
}

void MarketData::subscribe(std::string_view const &channel, std::span<Symbol const> const &symbols) {
  if (std::empty(symbols)) {
    return;
  }
  std::string message;
  fmt::format_to(
      std::back_inserter(message),
      R"({{)"
      R"("method":"subscribe",)"
      R"("params":{{)"
      R"("channel":"{}",)"
      R"("symbol":["{}"])"sv,
      channel,
      fmt::join(symbols, R"(",")"sv));
  if (channel == "book"sv) {
    if (shared_.settings.ws.public_subscribe_book_depth) {
      fmt::format_to(std::back_inserter(message), R"(,"depth":{})"sv, shared_.settings.ws.public_subscribe_book_depth);
    }
    fmt::format_to(std::back_inserter(message), R"(,"snapshot":true)"sv);
  }
  fmt::format_to(
      std::back_inserter(message),
      R"(}})"
      R"(}})"sv);
  log::info<3>(R"(request="{}")"sv, message);
  (*connection_).send_text(message);
}

void MarketData::parse(std::string_view const &message) {
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

void MarketData::operator()(Trace<json::Status> const &event) {
  auto &[trace_info, status] = event;
  log::info("status={}"sv, status);
  (*this)(ConnectionStatus::READY);
  if (index_ == 0) {
    subscribe_static();
  }
  subscribe();
}

void MarketData::operator()(Trace<json::Heartbeat> const &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<5>("heartbeat={}"sv, heartbeat);
  (*connection_).touch(trace_info.source_receive_time);
}

void MarketData::operator()(Trace<json::Error> const &event) {
  auto &[trace_info, error] = event;
  log::error("error={}"sv, error);
}

void MarketData::operator()(Trace<json::Pong> const &event) {
  auto &[trace_info, pong] = event;
  log::info<5>("pong={}"sv, pong);
  auto external_latency = trace_info.origin_create_time - std::chrono::nanoseconds{pong.req_id};
  log::warn("DEBUG external_latency={}"sv, external_latency);
  (*connection_).touch(trace_info.source_receive_time);
}

void MarketData::operator()(Trace<json::Subscribe> const &event) {
  auto &[trace_info, subscribe] = event;
  if (subscribe.success) {
    log::info<5>("subscribe={}"sv, subscribe);
  } else {
    log::error("subscribe={}"sv, subscribe);
  }
}

void MarketData::operator()(Trace<json::Instrument> const &event) {
  auto &[trace_info, instrument] = event;
  log::info<5>("instrument={}"sv, instrument);
  (*connection_).touch(trace_info.source_receive_time);
  std::vector<Symbol> symbols;
  symbols.reserve(std::size(instrument.data.pairs));
  size_t counter = 0;
  for (auto &item : instrument.data.pairs) {
    auto discard = [&]() {
      if (item.status == json::PairsStatus::DELISTED) {
        return true;
      }
      return shared_.discard_symbol(item.symbol);
    }();
    auto reference_data = ReferenceData{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .description = {},
        .security_type = SecurityType::SPOT,
        .cfi_code = {},
        .base_currency = item.base,
        .quote_currency = item.quote,
        .settlement_currency = {},
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = item.price_increment,
        .tick_size_steps = {},
        .multiplier = 1.0,  // ???
        .min_notional = NaN,
        .min_trade_vol = item.qty_increment,  // XXX FIXME qty_min
        .max_trade_vol = NaN,
        .trade_vol_step_size = item.qty_increment,
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
        .discard = discard,
    };
    create_trace_and_dispatch(handler_, trace_info, reference_data, true);
    if (discard) {
      continue;
    }
    if (all_symbols_.emplace(item.symbol).second) {  // only include new
      symbols.emplace_back(item.symbol);
    }
    ++counter;
    auto market_status = MarketStatus{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .trading_status = map(item.status),
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, market_status, true);
  }
  log::info("pairs {} / {}"sv, counter, std::size(instrument.data.pairs));
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

void MarketData::operator()(Trace<json::Ticker> const &event) {
  auto &[trace_info, ticker] = event;
  log::info<5>("ticker={}"sv, ticker);
  (*connection_).touch(trace_info.source_receive_time);
  for (auto &item : ticker.data) {
    auto top_of_book = TopOfBook{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .layer{
            .bid_price = item.bid,
            .bid_quantity = item.bid_qty,
            .ask_price = item.ask,
            .ask_quantity = item.ask_qty,
        },
        .update_type = map(ticker.type),
        .exchange_time_utc = item.timestamp,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
    std::array<Statistics, 3> statistics{{
        {
            .type = StatisticsType::TRADE_VOLUME,
            .value = item.volume,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::LOWEST_TRADED_PRICE,
            .value = item.low,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
        {
            .type = StatisticsType::HIGHEST_TRADED_PRICE,
            .value = item.high,
            .begin_time_utc = {},
            .end_time_utc = {},
        },
    }};
    auto statistics_update = StatisticsUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = item.symbol,
        .statistics = statistics,
        .update_type = map(ticker.type),
        .exchange_time_utc = item.timestamp,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, event.trace_info, statistics_update, true);
  }
}

void MarketData::operator()(Trace<json::Trade> const &event) {
  auto &[trace_info, trade] = event;
  log::info<5>("trade={}"sv, trade);
  (*connection_).touch(trace_info.source_receive_time);
  shared_.trades.clear();
  auto emplace_back = [&](auto &item) {
    auto trade_2 = Trade{
        .side = map(item.side),
        .price = item.price,
        .quantity = item.qty,
        .trade_id = {},
        .taker_order_id = {},
        .maker_order_id = {},
    };
    utils::charconv::to_string(std::back_inserter(trade_2.trade_id), item.trade_id);
    shared_.trades.emplace_back(trade_2);
  };
  std::string_view previous;
  std::chrono::nanoseconds exchange_time_utc = {};
  auto dispatch = [&]() {
    if (std::empty(shared_.trades)) {
      return;
    }
    assert(!std::empty(previous));
    auto trade_summary = TradeSummary{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = previous,
        .trades = shared_.trades,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
    shared_.trades.clear();
  };
  for (auto &item : trade.data) {
    if (item.symbol != previous) {
      dispatch();
    }
    emplace_back(item);
    previous = item.symbol;
    utils::update_max(exchange_time_utc, item.timestamp);
  }
  dispatch();
}

void MarketData::operator()(Trace<json::Book> const &event) {
  auto &[trace_info, book] = event;
  log::info<5>("book={}"sv, book);
  (*connection_).touch(trace_info.source_receive_time);
  shared_.bids.clear();
  shared_.asks.clear();
  auto emplace_back = [](auto &result, auto &value) {
    auto mbp_update = MBPUpdate{
        .price = value.price,
        .quantity = value.qty,
        .implied_quantity = NaN,
        .number_of_orders = {},
        .update_action = {},
        .price_level = {},
    };
    result.emplace_back(std::move(mbp_update));
  };
  for (auto &item : book.data) {
    for (auto &item_2 : item.bids) {
      emplace_back(shared_.bids, item_2);
    }
    for (auto &item_2 : item.asks) {
      emplace_back(shared_.asks, item_2);
    }
    if (!(std::empty(shared_.bids) && std::empty(shared_.asks))) {
      auto market_by_price_update = MarketByPriceUpdate{
          .stream_id = stream_id_,
          .exchange = shared_.settings.exchange,
          .symbol = item.symbol,
          .bids = shared_.bids,
          .asks = shared_.asks,
          .update_type = map(book.type),
          .exchange_time_utc = item.timestamp,
          .exchange_sequence = {},
          .sending_time_utc = {},
          .price_precision = {},
          .quantity_precision = {},
          .checksum = {},
      };
      create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true);
      shared_.bids.clear();
      shared_.asks.clear();
    }
  }
}

void MarketData::operator()(Trace<json::Balances> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::Executions> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::AddOrder> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::CancelOrder> const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(Trace<json::CancelAll> const &) {
  log::fatal("Unexpected"sv);
}

}  // namespace kraken
}  // namespace roq

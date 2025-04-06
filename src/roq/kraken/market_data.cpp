/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/kraken/market_data.hpp"

#include <algorithm>
#include <utility>

#include "roq/mask.hpp"

#include "roq/utils/update.hpp"

#include "roq/utils/metrics/factory.hpp"

#include "roq/web/socket/client.hpp"

#include "roq/core/tools/exception.hpp"

#include "roq/kraken/json/map.hpp"
#include "roq/kraken/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === CONSTANTS ===

namespace {
auto const NAME = "md"sv;

auto const SUPPORTS = Mask{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
};
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
      decode_buffer_(shared.settings.misc.decode_buffer_size),
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
}

void MarketData::operator()(metrics::Writer &writer) {
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
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
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
  (*this)(ConnectionStatus::READY);
  subscribe();
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

void MarketData::subscribe(std::span<Symbol const> const &symbols) {
  subscribe("trade"sv, symbols);
  subscribe("spread"sv, symbols);
  subscribe("book"sv, symbols);
}

void MarketData::subscribe(std::string_view const &name, std::span<Symbol const> const &symbols) {
  log::info(R"(subscribe name="{}", len(symbols)={})"sv, name, std::size(symbols));
  if (shared_.settings.ws.public_subscribe_book_depth && name.compare("book"sv) == 0) {
    auto message = fmt::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}",)"
        R"("depth":{})"
        R"(}})"
        R"(}})"sv,
        fmt::join(symbols, R"(",")"sv),
        name,
        shared_.settings.ws.public_subscribe_book_depth);
    log::info<3>(R"(request="{}")"sv, message);
    (*connection_).send_text(message);
  } else {
    auto message = fmt::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}")"
        R"(}})"
        R"(}})"sv,
        fmt::join(symbols, R"(",")"sv),
        name);
    log::info<3>(R"(request="{}")"sv, message);
    (*connection_).send_text(message);
  }
}

void MarketData::subscribe_book(std::string_view const &symbol) {
  auto message = fmt::format(
      R"({{)"
      R"("event":"subscribe",)"
      R"("pair":["{}"],)"
      R"("subscription":{{)"
      R"("name":"book",)"
      R"("depth":{})"
      R"(}})"
      R"(}})"sv,
      symbol,
      shared_.settings.ws.public_subscribe_book_depth);
  log::info<3>(R"(request="{}")"sv, message);
  (*connection_).send_text(message);
}

void MarketData::unsubscribe_book(std::string_view const &symbol) {
  auto message = fmt::format(
      R"({{)"
      R"("event":"unsubscribe",)"
      R"("pair":["{}"],)"
      R"("subscription":{{)"
      R"("name":"book",)"
      R"("depth":{})"
      R"(}})"
      R"(}})"sv,
      symbol,
      shared_.settings.ws.public_subscribe_book_depth);
  log::info<3>(R"(request="{}")"sv, message);
  (*connection_).send_text(message);
}

void MarketData::parse(std::string_view const &message) {
  profile_.parse([&]() {
    auto log_message = [&]() { log::warn(R"(message="{}")"sv, message); };
    TraceInfo trace_info;
    try {
      if (!json::ParserPublic::dispatch(*this, message, decode_buffer_, trace_info))
        log_message();
    } catch (...) {
      log_message();
      core::tools::UnhandledException::terminate();
    }
  });
}

void MarketData::operator()(Trace<json::Error> const &event) {
  auto &[trace_info, error] = event;
  log::fatal("error={}"sv, error);
}

void MarketData::operator()(Trace<json::SystemStatus> const &event) {
  auto &[trace_info, system_status] = event;
  log::info("system_status={}"sv, system_status);
}

void MarketData::operator()(Trace<json::Pong> const &event) {
  auto &[trace_info, pong] = event;
  log::info<1>("pong={}"sv, pong);
}

void MarketData::operator()(Trace<json::Heartbeat> const &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<1>("heartbeat={}"sv, heartbeat);
}

void MarketData::operator()(Trace<json::SubscriptionStatus> const &event) {
  auto &[trace_info, subscription_status] = event;
  log::info<1>("subscription_status={}"sv, subscription_status);
}

void MarketData::operator()(Trace<json::Trade> const &event, std::string_view const &pair) {
  auto &[trace_info, trade] = event;
  log::info<3>(R"(trade={}, pair="{}")"sv, trade, pair);
  (*connection_).touch(trace_info.source_receive_time);
  shared_.trades.clear();
  auto emplace_back = [](auto &result, auto &value) {
    auto trade = Trade{
        .side = map(value.side),
        .price = value.price,
        .quantity = value.volume,
        .trade_id = {},
        .taker_order_id = {},
        .maker_order_id = {},
    };
    result.emplace_back(std::move(trade));
  };
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : trade.data) {
    emplace_back(shared_.trades, item);
    utils::update_first(exchange_time_utc, item.time);
  }
  if (!std::empty(shared_.trades)) {
    auto trade_summary = TradeSummary{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = pair,
        .trades = shared_.trades,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
  }
}

void MarketData::operator()(Trace<json::Spread> const &event, std::string_view const &pair) {
  auto &[trace_info, spread] = event;
  log::info<3>(R"(spread={}, pair="{}")"sv, spread, pair);
  (*connection_).touch(trace_info.source_receive_time);
  auto top_of_book = TopOfBook{
      .stream_id = stream_id_,
      .exchange = shared_.settings.exchange,
      .symbol = pair,
      .layer{
          .bid_price = spread.bid,
          .bid_quantity = spread.bid_volume,
          .ask_price = spread.ask,
          .ask_quantity = spread.ask_volume,
      },
      .update_type = UpdateType::SNAPSHOT,
      .exchange_time_utc = spread.timestamp,
      .exchange_sequence = {},
      .sending_time_utc = {},
  };
  create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
}

void MarketData::operator()(Trace<json::Book> const &event, std::string_view const &pair) {
  auto &[trace_info, book] = event;
  log::info<3>(R"(book={}, pair="{}")"sv, book, pair);
  (*connection_).touch(trace_info.source_receive_time);
  bool snapshot = !std::empty(book.bs) && !std::empty(book.as);
  auto iter = latch_.find(pair);
  if (iter != std::end(latch_)) [[unlikely]] {
    if (!snapshot) {
      return;  //  waiting for snapshot
    } else {
      latch_.erase(iter);  // unlatch
      log::info(R"(DEBUG: unlatching symbol="{}")"sv, pair);
    }
  }
  bool live = !std::empty(book.b) && !std::empty(book.a);
  if (snapshot && live) [[unlikely]]
    log::fatal("Unexpected"sv);
  shared_.bids.clear();
  shared_.asks.clear();
  auto emplace_back = [](auto &result, auto &value) {
    auto mbp_update = MBPUpdate{
        .price = value.price,
        .quantity = value.volume,
        .implied_quantity = NaN,
        .number_of_orders = {},
        .update_action = {},
        .price_level = {},
    };
    result.emplace_back(std::move(mbp_update));
  };
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : book.b) {
    emplace_back(shared_.bids, item);
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.bs) {
    emplace_back(shared_.bids, item);
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.a) {
    emplace_back(shared_.asks, item);
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.as) {
    emplace_back(shared_.asks, item);
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  if (!(std::empty(shared_.bids) && std::empty(shared_.asks))) {
    auto market_by_price_update = MarketByPriceUpdate{
        .stream_id = stream_id_,
        .exchange = shared_.settings.exchange,
        .symbol = pair,
        .bids = shared_.bids,
        .asks = shared_.asks,
        .update_type = snapshot ? UpdateType::SNAPSHOT : UpdateType::INCREMENTAL,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .sending_time_utc = {},
        .price_precision = {},
        .quantity_precision = {},
        .checksum = {},
    };
    try {
      create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true);
    } catch (BadState &) {
      resubscribe(trace_info, pair);
    }
  }
}

void MarketData::resubscribe(TraceInfo const &trace_info, std::string_view const &symbol) {
  log::warn<1>(R"(*** RESUBSCRIBE *** (symbol="{}"))"sv, symbol);
  auto market_by_price_update = MarketByPriceUpdate{
      .stream_id = stream_id_,
      .exchange = shared_.settings.exchange,
      .symbol = symbol,
      .bids = {},
      .asks = {},
      .update_type = UpdateType::STALE,
      .exchange_time_utc = {},
      .exchange_sequence = {},
      .sending_time_utc = {},
      .price_precision = {},
      .quantity_precision = {},
      .checksum = {},
  };
  log::info<3>("market_by_price_update={}"sv, market_by_price_update);
  create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true);
  latch_.emplace(symbol);  // latch
  log::info(R"(DEBUG: latching symbol="{}")"sv, symbol);
  unsubscribe_book(symbol);
  subscribe_book(symbol);
}

}  // namespace kraken
}  // namespace roq

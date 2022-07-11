/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/market_data.hpp"

#include <algorithm>

#include "roq/mask.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/back_emplacer.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/kraken/flags.hpp"

#include "roq/kraken/json/utils.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

namespace {
auto const NAME = "md"sv;
const Mask SUPPORTS{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(std::string_view const &group, std::string_view const &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::ws_public_uri();
  core::web::ClientSocket::Config config{
      .always_reconnect = true,
      .connection_timeout = server::Flags::net_connection_timeout(),
      .disconnect_on_idle_timeout = server::Flags::net_disconnect_on_idle_timeout(),
      .validate_certificate = server::Flags::net_tls_validate_certificate(),
      .uris = {&uri, 1},
      .query = {},
      .ping_frequency = Flags::ws_public_ping_freq(),
      .read_buffer_size = Flags::decode_buffer_size(),  // XXX need read buffer size
      .encode_buffer_size = Flags::encode_buffer_size(),
  };
  return core::web::ClientSocket{handler, context, config, []() { return std::string(); }};
}

template <typename T>
void emplace(MBPUpdate &result, const T &value) {
  new (&result) MBPUpdate{
      .price = value.price,
      .quantity = value.volume,
      .implied_quantity = NaN,
      .number_of_orders = {},
      .update_action = {},
      .price_level = {},
  };
}

template <typename T>
void emplace(Trade &result, const T &value) {
  new (&result) Trade{
      .side = json::map(value.side),
      .price = value.price,
      .quantity = value.volume,
      .trade_id = {},
  };
}
}  // namespace

MarketData::MarketData(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared, size_t index)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)), index_(index),
      connection_(create_connection(*this, context)), decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
          .heartbeat = create_metrics(name_, "heartbeat"sv),
      },
      shared_(shared) {
}

void MarketData::operator()(Event<Start> const &) {
  connection_.start();
}

void MarketData::operator()(Event<Stop> const &) {
  connection_.stop();
}

void MarketData::operator()(Event<Timer> const &event) {
  connection_.refresh(event.value.now);
}

void MarketData::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.parse, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY)
      .write(latency_.heartbeat, metrics::LATENCY);
}

void MarketData::subscribe(size_t start_from) {
  if (ready())
    subscribe(shared_.symbols.get_slice(index_, start_from));
}

void MarketData::operator()(core::web::ClientSocket::Connected const &) {
  // note! wait for upgrade
}

void MarketData::operator()(core::web::ClientSocket::Disconnected const &) {
  ++counter_.disconnect;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
}

void MarketData::operator()(core::web::ClientSocket::Ready const &) {
  (*this)(ConnectionStatus::READY);
  subscribe();
}

void MarketData::operator()(core::web::ClientSocket::Close const &) {
}

void MarketData::operator()(core::web::ClientSocket::Latency const &latency) {
  auto trace_info = server::create_trace_info();
  const ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(core::web::ClientSocket::Text const &text) {
  parse(text.payload);
}

void MarketData::operator()(core::web::ClientSocket::Binary const &) {
  log::fatal("Unexpected"sv);
}

void MarketData::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    const StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::WS,
        .encoding = {Encoding::JSON},
        .priority = Priority::PRIMARY,
        .connection_status = status_,
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
  if (Flags::ws_public_subscribe_book_depth() && name.compare("book"sv) == 0) {
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
        Flags::ws_public_subscribe_book_depth());
    log::info<3>(R"(request="{}")"sv, message);
    connection_.send_text(message);
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
    connection_.send_text(message);
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
      Flags::ws_public_subscribe_book_depth());
  log::info<3>(R"(request="{}")"sv, message);
  connection_.send_text(message);
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
      Flags::ws_public_subscribe_book_depth());
  log::info<3>(R"(request="{}")"sv, message);
  connection_.send_text(message);
}

void MarketData::parse(std::string_view const &message) {
  profile_.parse([&]() {
    auto trace_info = server::create_trace_info();
    core::json::Buffer buffer(decode_buffer_);
    auto result = json::ParserPublic::dispatch(*this, message, buffer, trace_info);
    if (!result) [[unlikely]]
      log::warn(R"(Unexpected: message="{}")"sv, message);
  });
}

void MarketData::operator()(Trace<json::Error const> const &event) {
  auto &[trace_info, error] = event;
  log::fatal("error={}"sv, error);
}

void MarketData::operator()(Trace<json::SystemStatus const> const &event) {
  auto &[trace_info, system_status] = event;
  log::info("system_status={}"sv, system_status);
}

void MarketData::operator()(Trace<json::Pong const> const &event) {
  auto &[trace_info, pong] = event;
  log::info<1>("pong={}"sv, pong);
}

void MarketData::operator()(Trace<json::Heartbeat const> const &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<1>("heartbeat={}"sv, heartbeat);
}

void MarketData::operator()(Trace<json::SubscriptionStatus const> const &event) {
  auto &[trace_info, subscription_status] = event;
  log::info<1>("subscription_status={}"sv, subscription_status);
}

void MarketData::operator()(Trace<json::Trade const> const &event, std::string_view const &pair) {
  auto &[trace_info, trade] = event;
  log::info<3>(R"(trade={}, pair="{}")"sv, trade, pair);
  connection_.touch(trace_info.source_receive_time);
  core::back_emplacer trades(shared_.trades);
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : trade.data) {
    trades.emplace_back([&item](auto &result) { emplace(result, item); });
    utils::update_first(exchange_time_utc, item.time);
  }
  if (!std::empty(trades)) {
    const TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = pair,
        .trades = trades,
        .exchange_time_utc = exchange_time_utc,
    };
    create_trace_and_dispatch(handler_, trace_info, trade_summary, true);
  }
}

void MarketData::operator()(Trace<json::Spread const> const &event, std::string_view const &pair) {
  auto &[trace_info, spread] = event;
  log::info<3>(R"(spread={}, pair="{}")"sv, spread, pair);
  connection_.touch(trace_info.source_receive_time);
  const TopOfBook top_of_book{
      .stream_id = stream_id_,
      .exchange = Flags::exchange(),
      .symbol = pair,
      .layer{
          .bid_price = spread.bid,
          .bid_quantity = spread.bid_volume,
          .ask_price = spread.ask,
          .ask_quantity = spread.ask_volume,
      },
      .update_type = UpdateType::INCREMENTAL,
      .exchange_time_utc = spread.timestamp,
      .exchange_sequence = {},
  };
  create_trace_and_dispatch(handler_, trace_info, top_of_book, true);
}

void MarketData::operator()(Trace<json::Book const> const &event, std::string_view const &pair) {
  auto &[trace_info, book] = event;
  log::info<3>(R"(book={}, pair="{}")"sv, book, pair);
  connection_.touch(trace_info.source_receive_time);
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
  core::back_emplacer bids(shared_.bids), asks(shared_.asks);
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : book.b) {
    bids.emplace_back([&item](auto &result) { emplace(result, item); });
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.bs) {
    bids.emplace_back([&item](auto &result) { emplace(result, item); });
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.a) {
    asks.emplace_back([&item](auto &result) { emplace(result, item); });
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  for (auto &item : book.as) {
    asks.emplace_back([&item](auto &result) { emplace(result, item); });
    utils::update_first(exchange_time_utc, item.timestamp);
  }
  if (!(std::empty(bids) && std::empty(asks))) {
    const MarketByPriceUpdate market_by_price_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = pair,
        .bids = bids,
        .asks = asks,
        .update_type = snapshot ? UpdateType::SNAPSHOT : UpdateType::INCREMENTAL,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
        .price_decimals = {},
        .quantity_decimals = {},
        .checksum = {},
    };
    try {
      create_trace_and_dispatch(handler_, trace_info, market_by_price_update, true, false);
    } catch (BadState &) {
      resubscribe(trace_info, pair);
    }
  }
}

void MarketData::resubscribe(TraceInfo const &trace_info, std::string_view const &symbol) {
  log::warn<1>(R"(*** RESUBSCRIBE *** (symbol="{}"))"sv, symbol);
  const MarketByPriceUpdate market_by_price_update{
      .stream_id = stream_id_,
      .exchange = Flags::exchange(),
      .symbol = symbol,
      .bids = {},
      .asks = {},
      .update_type = UpdateType::STALE,
      .exchange_time_utc = {},
      .exchange_sequence = {},
      .price_decimals = {},
      .quantity_decimals = {},
      .checksum = {},
  };
  log::info<3>("market_by_price_update={}"sv, market_by_price_update);
  create_trace_and_dispatch(
      shared_,
      trace_info,
      market_by_price_update,
      true,
      false,
      shared_.final_bids,
      shared_.final_asks,
      []([[maybe_unused]] auto &market_by_price) {});
  latch_.emplace(symbol);  // latch
  log::info(R"(DEBUG: latching symbol="{}")"sv, symbol);
  unsubscribe_book(symbol);
  subscribe_book(symbol);
}

}  // namespace kraken
}  // namespace roq

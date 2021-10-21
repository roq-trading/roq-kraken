/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/market_data.h"

#include <algorithm>

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/back_emplacer.h"

#include "roq/core/metrics/factory.h"

#include "roq/kraken/flags.h"

#include "roq/kraken/json/utils.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

namespace {
static const auto NAME = "md"_sv;
static const auto SUPPORTS = utils::Mask{
    SupportType::TOP_OF_BOOK,
    SupportType::MARKET_BY_PRICE,
    SupportType::TRADE_SUMMARY,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

template <typename T>
void emplace(MBPUpdate &result, const T &value) {
  new (&result) MBPUpdate{
      .price = value.price,
      .quantity = value.volume,
      .implied_quantity = NaN,
      .price_level = {},
      .number_of_orders = {},
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

MarketData::MarketData(
    Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"_sv, stream_id_, NAME)),
      connection_(
          *this,
          context,
          core::URI(Flags::ws_public_uri()),
          {},  // query
          Flags::ws_public_ping_freq(),
          Flags::decode_buffer_size(),  // XXX need read buffer size
          Flags::encode_buffer_size(),
          []() { return std::string(); }),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .parse = create_metrics(name_, "parse"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
          .heartbeat = create_metrics(name_, "heartbeat"_sv),
      },
      shared_(shared), download_(Flags::ws_public_request_timeout(), [this](auto state) {
        return download(state);
      }) {
}

void MarketData::operator()(const Event<Start> &) {
  connection_.start();
}

void MarketData::operator()(const Event<Stop> &) {
  connection_.stop();
}

void MarketData::operator()(const Event<Timer> &event) {
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

void MarketData::update_subscriptions(std::vector<std::string> &symbols) {
  assert(&symbols != &symbols_);
  auto max_size = Flags::ws_public_max_subscriptions_per_stream();
  auto offset = symbols_.size();
  if (max_size <= offset)
    return;
  if (symbols.empty())
    return;
  symbols_.reserve(max_size);
  auto length = std::min(max_size - offset, symbols.size());
  assert(length > 0);
  for (size_t i = {}; i < length; ++i) {
    symbols_.emplace_back(symbols.back());
    symbols.pop_back();
  }
  assert(length == (symbols_.size() - offset));
  if (ready_)
    subscribe({&symbols_[offset], length});
}

void MarketData::operator()(const core::web::Socket::Connected &) {
  // note! wait for upgrade
}

void MarketData::operator()(const core::web::Socket::Disconnected &) {
  ++counter_.disconnect;
  ready_ = false;
  next_heartbeat_ = {};
  (*this)(ConnectionStatus::DISCONNECTED);
  download_.reset();
}

void MarketData::operator()(const core::web::Socket::Ready &) {
  (*this)(ConnectionStatus::DOWNLOADING);
  download_.begin();
}

void MarketData::operator()(const core::web::Socket::Close &) {
}

void MarketData::operator()(const core::web::Socket::Latency &latency) {
  server::TraceInfo trace_info;
  const ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

void MarketData::operator()(const core::web::Socket::Text &text) {
  parse(text.payload);
}

void MarketData::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    server::TraceInfo trace_info;
    const StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::WEB_SOCKET,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_sv, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
  }
}

uint32_t MarketData::download(MarketDataState state) {
  switch (state) {
    case MarketDataState::UNDEFINED:
      assert(false);
      break;
    case MarketDataState::SUBSCRIBE:
      subscribe(symbols_);
      return {};
    case MarketDataState::DONE:
      (*this)(ConnectionStatus::READY);
      assert(!ready_);
      ready_ = true;
      return {};
  }
  assert(false);
  return {};
}

void MarketData::subscribe(const roq::span<std::string> &symbols) {
  subscribe("trade"_sv, symbols);
  subscribe("spread"_sv, symbols);
  subscribe("book"_sv, symbols);
}

void MarketData::subscribe(const std::string_view &name, const roq::span<std::string> &symbols) {
  log::info(R"(subscribe name="{}", len(symbols)={})"_sv, name, std::size(symbols));
  if (Flags::ws_public_subscribe_book_depth() && name.compare("book"_sv) == 0) {
    auto message = fmt::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}",)"
        R"("depth":{})"
        R"(}})"
        R"(}})"_sv,
        fmt::join(symbols, R"(",")"_sv),
        name,
        Flags::ws_public_subscribe_book_depth());
    log::info<3>(R"(request="{}")"_sv, message);
    connection_.send_text(message);
  } else {
    auto message = fmt::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}")"
        R"(}})"
        R"(}})"_sv,
        fmt::join(symbols, R"(",")"_sv),
        name);
    log::info<3>(R"(request="{}")"_sv, message);
    connection_.send_text(message);
  }
}

void MarketData::subscribe_book(const std::string_view &symbol) {
  auto message = fmt::format(
      R"({{)"
      R"("event":"subscribe",)"
      R"("pair":["{}"],)"
      R"("subscription":{{)"
      R"("name":"book",)"
      R"("depth":{})"
      R"(}})"
      R"(}})"_sv,
      symbol,
      Flags::ws_public_subscribe_book_depth());
  log::info<3>(R"(request="{}")"_sv, message);
  connection_.send_text(message);
}

void MarketData::unsubscribe_book(const std::string_view &symbol) {
  auto message = fmt::format(
      R"({{)"
      R"("event":"unsubscribe",)"
      R"("pair":["{}"],)"
      R"("subscription":{{)"
      R"("name":"book",)"
      R"("depth":{})"
      R"(}})"
      R"(}})"_sv,
      symbol,
      Flags::ws_public_subscribe_book_depth());
  log::info<3>(R"(request="{}")"_sv, message);
  connection_.send_text(message);
}

void MarketData::parse(const std::string_view &message) {
  profile_.parse([&]() {
    server::TraceInfo trace_info;
    core::json::Buffer buffer(decode_buffer_);
    auto result = json::ParserPublic::dispatch(*this, message, buffer, trace_info);
    if (ROQ_UNLIKELY(!result))
      log::warn(R"(Unexpected: message="{}")"_sv, message);
  });
}

void MarketData::operator()(const server::Trace<json::Error> &event) {
  auto &[trace_info, error] = event;
  log::fatal("error={}"_sv, error);
}

void MarketData::operator()(const server::Trace<json::SystemStatus> &event) {
  auto &[trace_info, system_status] = event;
  log::info("system_status={}"_sv, system_status);
}

void MarketData::operator()(const server::Trace<json::Pong> &event) {
  auto &[trace_info, pong] = event;
  log::info<1>("pong={}"_sv, pong);
}

void MarketData::operator()(const server::Trace<json::Heartbeat> &event) {
  auto &[trace_info, heartbeat] = event;
  log::info<1>("heartbeat={}"_sv, heartbeat);
}

void MarketData::operator()(const server::Trace<json::SubscriptionStatus> &event) {
  auto &[trace_info, subscription_status] = event;
  log::info<1>("subscription_status={}"_sv, subscription_status);
}

void MarketData::operator()(const server::Trace<json::Trade> &event, const std::string_view &pair) {
  auto &[trace_info, trade] = event;
  log::info<3>(R"(trade={}, pair="{}")"_sv, trade, pair);
  core::back_emplacer trades(shared_.trades);
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : trade.data) {
    trades.emplace_back([&item](auto &result) { emplace(result, item); });
    utils::update_first(exchange_time_utc, item.time);
  }
  if (!trades.empty()) {
    const TradeSummary trade_summary{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = pair,
        .trades = trades,
        .exchange_time_utc = exchange_time_utc,
    };
    server::create_trace_and_dispatch(trace_info, trade_summary, handler_, true);
  }
}

void MarketData::operator()(
    const server::Trace<json::Spread> &event, const std::string_view &pair) {
  auto &[trace_info, spread] = event;
  log::info<3>(R"(spread={}, pair="{}")"_sv, spread, pair);
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
  };
  server::create_trace_and_dispatch(trace_info, top_of_book, handler_, true);
}

void MarketData::operator()(const server::Trace<json::Book> &event, const std::string_view &pair) {
  auto &[trace_info, book] = event;
  log::info<3>(R"(book={}, pair="{}")"_sv, book, pair);
  bool snapshot = !book.bs.empty() && !book.as.empty();
  auto iter = latch_.find(pair);
  if (ROQ_UNLIKELY(iter != latch_.end())) {
    if (!snapshot) {
      return;  //  waiting for snapshot
    } else {
      latch_.erase(iter);  // unlatch
      log::info(R"(DEBUG: unlatching symbol="{}")"_sv, pair);
    }
  }
  bool live = !book.b.empty() && !book.a.empty();
  if (ROQ_UNLIKELY(snapshot && live))
    log::fatal("Unexpected"_sv);
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
  if (!(bids.empty() && asks.empty())) {
    const MarketByPriceUpdate market_by_price_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = pair,
        .bids = bids,
        .asks = asks,
        .update_type = snapshot ? UpdateType::SNAPSHOT : UpdateType::INCREMENTAL,
        .exchange_time_utc = exchange_time_utc,
        .exchange_sequence = {},
    };
    try {
      server::create_trace_and_dispatch(trace_info, market_by_price_update, handler_, true, false);
    } catch (market::BadState &) {
      resubscribe(trace_info, pair);
    }
  }
}

void MarketData::resubscribe(const server::TraceInfo &trace_info, const std::string_view &symbol) {
  log::warn<1>(R"(*** RESUBSCRIBE *** (symbol="{}"))"_sv, symbol);
  MarketByPriceUpdate market_by_price_update{
      .stream_id = stream_id_,
      .exchange = Flags::exchange(),
      .symbol = symbol,
      .bids = {},
      .asks = {},
      .update_type = UpdateType::STALE,
      .exchange_time_utc = {},
      .exchange_sequence = {},
  };
  log::info<3>("market_by_price_update={}"_sv, market_by_price_update);
  server::create_trace_and_dispatch(
      trace_info,
      market_by_price_update,
      shared_,
      true,
      false,
      shared_.final_bids,
      shared_.final_asks,
      []([[maybe_unused]] auto &market_by_price) {});
  latch_.emplace(symbol);  // latch
  log::info(R"(DEBUG: latching symbol="{}")"_sv, symbol);
  unsubscribe_book(symbol);
  subscribe_book(symbol);
}

}  // namespace kraken
}  // namespace roq

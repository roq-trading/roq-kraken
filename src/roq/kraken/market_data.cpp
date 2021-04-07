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
    : handler_(handler), stream_id_(stream_id), name_(roq::format("{}:{}"_fmt, stream_id_, NAME)),
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
  assert(length > 0u);
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
  ExternalLatency external_latency{
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
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .type = StreamType::WEB_SOCKET,
        .supports = SUPPORTS.get(),
        .account = {},
        .priority = Priority::PRIMARY,
        .status = status_,
    };
    log::info("stream_status={}"_fmt, stream_status);
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
  log::info(R"(subscribe name="{}", len(symbols)={})"_fmt, name, std::size(symbols));
  if (Flags::ws_public_subscribe_book_depth() && name.compare("book"_sv) == 0) {
    auto message = roq::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}",)"
        R"("depth":{})"
        R"(}})"
        R"(}})"_fmt,
        roq::join(symbols, R"(",")"_sv),
        name,
        Flags::ws_public_subscribe_book_depth());
    log::trace_3(R"(request="{}")"_fmt, message);
    connection_.send_text(message);
  } else {
    auto message = roq::format(
        R"({{)"
        R"("event":"subscribe",)"
        R"("pair":["{}"],)"
        R"("subscription":{{)"
        R"("name":"{}")"
        R"(}})"
        R"(}})"_fmt,
        roq::join(symbols, R"(",")"_sv),
        name);
    log::trace_3(R"(request="{}")"_fmt, message);
    connection_.send_text(message);
  }
}

void MarketData::parse(const std::string_view &message) {
  profile_.parse([&]() {
    server::TraceInfo trace_info;
    core::json::Buffer buffer(decode_buffer_);
    auto result = json::ParserPublic::dispatch(*this, message, buffer, trace_info);
    if (ROQ_UNLIKELY(!result))
      log::warn(R"(Unexpected: message="{}")"_fmt, message);
  });
}

void MarketData::operator()(const json::Error &error, const server::TraceInfo &) {
  log::fatal("error={}"_fmt, error);
}

void MarketData::operator()(const json::SystemStatus &system_status, const server::TraceInfo &) {
  log::info("system_status={}"_fmt, system_status);
}

void MarketData::operator()(const json::Pong &pong, const server::TraceInfo &) {
  log::trace_1("pong={}"_fmt, pong);
}

void MarketData::operator()(const json::Heartbeat &heartbeat, const server::TraceInfo &) {
  log::trace_1("heartbeat={}"_fmt, heartbeat);
}

void MarketData::operator()(
    const json::SubscriptionStatus &subscription_status, const server::TraceInfo &) {
  log::trace_1("subscription_status={}"_fmt, subscription_status);
}

void MarketData::operator()(
    const json::Trade &trade, const std::string_view &pair, const server::TraceInfo &trace_info) {
  log::trace_3(R"(trade={}, pair="{}")"_fmt, trade, pair);
  core::back_emplacer trades(shared_.trades);
  std::chrono::nanoseconds exchange_time_utc = {};
  for (auto &item : trade.data) {
    trades.emplace_back([&item](auto &result) { emplace(result, item); });
    utils::update_first(exchange_time_utc, item.time);
  }
  if (!trades.empty()) {
    TradeSummary trade_summary{
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
    const json::Spread &spread, const std::string_view &pair, const server::TraceInfo &trace_info) {
  log::trace_3(R"(spread={}, pair="{}")"_fmt, spread, pair);
  TopOfBook top_of_book{
      .stream_id = stream_id_,
      .exchange = Flags::exchange(),
      .symbol = pair,
      .layer{
          .bid_price = spread.bid,
          .bid_quantity = spread.bid_volume,
          .ask_price = spread.ask,
          .ask_quantity = spread.ask_volume,
      },
      .snapshot = false,  // note! we don't know... false is probably ok
      .exchange_time_utc = spread.timestamp,
  };
  server::create_trace_and_dispatch(trace_info, top_of_book, handler_, true);
}

void MarketData::operator()(
    const json::Book &book, const std::string_view &pair, const server::TraceInfo &trace_info) {
  log::trace_3(R"(book={}, pair="{}")"_fmt, book, pair);
  bool snapshot = !book.bs.empty() && !book.as.empty();
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
    MarketByPriceUpdate market_by_price_update{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = pair,
        .bids = bids,
        .asks = asks,
        .snapshot = snapshot,
        .exchange_time_utc = exchange_time_utc,
    };
    server::create_trace_and_dispatch(trace_info, market_by_price_update, handler_, true);
  }
}

}  // namespace kraken
}  // namespace roq

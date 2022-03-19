/* Copyright (c) 2017-2022, Hans Erik Thrane */

#include "roq/kraken/rest.hpp"

#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/kraken/flags.hpp"

#include "roq/kraken/json/result.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

namespace {
const auto NAME = "rest"sv;

const auto SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};

auto create_connection(auto &handler, auto &context) {
  auto uri = Flags::rest_uri();
  core::web::Client::Config config{
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
      .validate_certificate = server::Flags::tls_validate_certificate(),
      .uris = {&uri, 1},
      .proxy = Flags::rest_proxy(),
      .user_agent = ROQ_PACKAGE_NAME,
      .connection = core::http::Connection::KEEP_ALIVE,
      .allow_pipelining = true,
      .request_timeout = Flags::rest_request_timeout(),
      .ping_frequency = Flags::rest_ping_freq(),
      .ping_path = Flags::rest_ping_path(),
  };
  return core::web::Client{handler, context, config};
}
}  // namespace

Rest::Rest(Handler &handler, core::io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_(handler), stream_id_(stream_id), name_(fmt::format("{}:{}"sv, stream_id_, NAME)),
      connection_(create_connection(*this, context)), decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .assets = create_metrics(name_, "assets"sv),
          .assets_ack = create_metrics(name_, "assets_ack"sv),
          .asset_pairs = create_metrics(name_, "asset_pairs"sv),
          .asset_pairs_ack = create_metrics(name_, "asset_pairs_ack"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      shared_(shared),
      download_(Flags::rest_request_timeout(), [this](auto state) { return download(state); }) {
}

void Rest::operator()(const Event<Start> &) {
  connection_.start();
}

void Rest::operator()(const Event<Stop> &) {
  connection_.stop();
}

void Rest::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void Rest::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.assets, metrics::PROFILE)
      .write(profile_.assets_ack, metrics::PROFILE)
      .write(profile_.asset_pairs, metrics::PROFILE)
      .write(profile_.asset_pairs_ack, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

void Rest::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS.get(),
        .status = status_,
        .type = StreamType::REST,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void Rest::operator()(const core::web::Client::Connected &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void Rest::operator()(const core::web::Client::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void Rest::operator()(const core::web::Client::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

uint32_t Rest::download(RestState state) {
  switch (state) {
    case RestState::UNDEFINED:
      assert(false);
      break;
    case RestState::ASSETS:
      get_assets();
      return 1;
    case RestState::ASSET_PAIRS:
      get_asset_pairs();
      return 1;
    case RestState::DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// assets

void Rest::get_assets() {
  profile_.assets([&]() {
    auto method = core::http::Method::GET;
    auto path = "/0/public/Assets"sv;
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto sequence = download_.sequence();
    connection_(
        "assets"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          auto trace_info = server::create_trace_info();
          server::Trace event(trace_info, response);
          get_assets_ack(event, sequence);
        });
  });
}

void Rest::get_assets_ack(const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.assets_ack([&]() {
    auto &[trace_info, response] = event;
    auto state = RestState::ASSETS;
    try {
      auto [status, category, body] = response.result();
      log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
      if (download_.skip(sequence, state)) {
        log::info("Download state={} has already been processed"sv, state);
        return;
      }
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      auto assets = core::json::Parser::create<json::Assets>(body, buffer);
      server::Trace event(trace_info, assets);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void Rest::operator()(const server::Trace<json::Assets> &event) {
  auto &[trace_info, assets] = event;
  log::info<4>("assets={}"sv, assets);
  // do nothing
}

// asset-pairs

void Rest::get_asset_pairs() {
  profile_.asset_pairs([&]() {
    auto method = core::http::Method::GET;
    auto path = "/0/public/AssetPairs"sv;
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto sequence = download_.sequence();
    connection_(
        "asset_pairs"sv,
        request,
        [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          auto trace_info = server::create_trace_info();
          server::Trace event(trace_info, response);
          get_asset_pairs_ack(event, sequence);
        });
  });
}

void Rest::get_asset_pairs_ack(const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.asset_pairs_ack([&]() {
    auto &[trace_info, response] = event;
    auto state = RestState::ASSET_PAIRS;
    try {
      auto [status, category, body] = response.result();
      log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
      if (download_.skip(sequence, state)) {
        log::info("Download state={} has already been processed"sv, state);
        return;
      }
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      auto asset_pairs = core::json::Parser::create<json::AssetPairs>(body, buffer);
      server::Trace event(trace_info, asset_pairs);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void Rest::operator()(const server::Trace<json::AssetPairs> &event) {
  auto &[trace_info, asset_pairs] = event;
  log::info<4>("asset_pairs={}"sv, asset_pairs);
  assert(std::empty(asset_pairs.error));
  std::vector<Symbol> symbols;
  symbols.reserve(std::size(asset_pairs.result));
  size_t counter = {};
  for (auto &item : asset_pairs.result) {
    log::info<2>("item={}"sv, item);
    if (std::empty(item.wsname)) {
      log::info<1>(R"(Skipping altname="{}", reason: wsname is empty)"sv, item.altname);
      continue;
    }
    std::string symbol(item.wsname);
    // remove escape
    symbol.erase(std::remove(std::begin(symbol), std::end(symbol), '\\'), std::end(symbol));
    if (shared_.discard_symbol(symbol))
      continue;
    if (all_symbols_.emplace(symbol).second)  // only include new
      symbols.emplace_back(symbol);
    ++counter;
    auto tick_size = std::pow(double{10.0}, -static_cast<double>(item.pair_decimals));
    auto min_trade_vol = std::pow(double{10.0}, -static_cast<double>(item.lot_decimals));
    ReferenceData reference_data{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .description = item.altname,
        .security_type = {},
        .base_currency = item.aclass_base,
        .quote_currency = item.aclass_quote,
        .margin_currency = {},
        .commission_currency = item.aclass_base,
        .tick_size = tick_size,
        .multiplier = item.lot_multiplier,  // XXX check
        .min_trade_vol = min_trade_vol,
        .max_trade_vol = NaN,
        .trade_vol_step_size = min_trade_vol,
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
    };
    server::create_trace_and_dispatch(handler_, trace_info, reference_data, true);
    MarketStatus market_status{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .trading_status = TradingStatus::OPEN,  // XXX doesn't exist?
    };
    server::create_trace_and_dispatch(handler_, trace_info, market_status, true);
  }
  log::info("AssetPairs {} / {}"sv, counter, std::size(asset_pairs.result));
  if (!std::empty(symbols)) {
    SymbolsUpdate symbols_update{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

}  // namespace kraken
}  // namespace roq

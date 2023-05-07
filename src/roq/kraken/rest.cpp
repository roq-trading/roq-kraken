/* Copyright (c) 2017-2023, Hans Erik Thrane */

#include "roq/kraken/rest.hpp"

#include <utility>

#include "roq/mask.hpp"
#include "roq/utils/update.hpp"

#include "roq/core/json/parser.hpp"

#include "roq/core/metrics/factory.hpp"

#include "roq/web/rest/client_factory.hpp"

#include "roq/kraken/flags.hpp"

#include "roq/kraken/json/result.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {

// === CONSTANTS ===

namespace {
auto const NAME = "rest"sv;

auto const SUPPORTS = Mask{
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};
}  // namespace

// === HELPERS ===

namespace {
auto create_name(auto stream_id) {
  return fmt::format("{}:{}"sv, stream_id, NAME);
}

auto create_connection(auto &handler, auto &settings, auto &context) {
  auto uri = Flags::rest_uri();
  auto config = web::rest::Client::Config{
      // connection
      .interface = {},
      .uris = {&uri, 1},
      .validate_certificate = settings.net.tls_validate_certificate,
      // connection manager
      .connection_timeout = {},
      .disconnect_on_idle_timeout = {},
      .connection = web::http::Connection::KEEP_ALIVE,
      // proxy
      .proxy = Flags::rest_proxy(),
      // http
      .query = {},
      .user_agent = ROQ_PACKAGE_NAME,
      .request_timeout = Flags::rest_request_timeout(),
      .ping_frequency = Flags::rest_ping_freq(),
      .ping_path = Flags::rest_ping_path(),
      // implementation
      .decode_buffer_size = Flags::decode_buffer_size(),
      .encode_buffer_size = Flags::encode_buffer_size(),
      .allow_pipelining = true,
  };
  return web::rest::ClientFactory::create(handler, context, config);
}

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(auto &settings, auto const &group, auto const &function)
      : core::metrics::Factory(settings.app.name, group, function) {}
};
}  // namespace

// === IMPLEMENTATION ===

Rest::Rest(Handler &handler, io::Context &context, uint16_t stream_id, Shared &shared)
    : handler_{handler}, stream_id_{stream_id}, name_{create_name(stream_id_)},
      connection_{create_connection(*this, shared.settings, context)}, decode_buffer_{Flags::decode_buffer_size()},
      counter_{
          .disconnect = create_metrics(shared.settings, name_, "disconnect"sv),
      },
      profile_{
          .assets = create_metrics(shared.settings, name_, "assets"sv),
          .assets_ack = create_metrics(shared.settings, name_, "assets_ack"sv),
          .asset_pairs = create_metrics(shared.settings, name_, "asset_pairs"sv),
          .asset_pairs_ack = create_metrics(shared.settings, name_, "asset_pairs_ack"sv),
      },
      latency_{
          .ping = create_metrics(shared.settings, name_, "ping"sv),
      },
      shared_{shared}, download_{Flags::rest_request_timeout(), [this](auto state) { return download(state); }} {
}

void Rest::operator()(Event<Start> const &) {
  (*connection_).start();
}

void Rest::operator()(Event<Stop> const &) {
  (*connection_).stop();
}

void Rest::operator()(Event<Timer> const &event) {
  (*connection_).refresh(event.value.now);
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
    TraceInfo trace_info;
    auto stream_status = StreamStatus{
        .stream_id = stream_id_,
        .account = {},
        .supports = SUPPORTS,
        .transport = Transport::TCP,
        .protocol = Protocol::HTTP,
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

void Rest::operator()(Trace<web::rest::Client::Connected> const &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void Rest::operator()(Trace<web::rest::Client::Disconnected> const &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void Rest::operator()(Trace<web::rest::Client::Latency> const &event) {
  auto &[trace_info, latency] = event;
  auto external_latency = ExternalLatency{
      .stream_id = stream_id_,
      .account = {},
      .latency = latency.sample,
  };
  create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

void Rest::operator()(
    Trace<web::rest::Response> const &, [[maybe_unused]] uint64_t request_id, [[maybe_unused]] uint64_t opaque) {
}

uint32_t Rest::download(RestState state) {
  switch (state) {
    using enum RestState;
    case UNDEFINED:
      assert(false);
      break;
    case ASSETS:
      get_assets();
      return 1;
    case ASSET_PAIRS:
      get_asset_pairs();
      return 1;
    case DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// assets

void Rest::get_assets() {
  profile_.assets([&]() {
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = "/0/public/Assets"sv,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_assets_ack(event, sequence);
    };
    (*connection_)("assets"sv, request, callback);
  });
}

void Rest::get_assets_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  constexpr auto const STATE = RestState::ASSETS;
  profile_.assets_ack([&]() {
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::Assets assets{body, decode_buffer_};
        Trace event_2{event, assets};
        (*this)(event_2);
        download_.check(STATE);
      }
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      download_.retry(STATE);
    };
    process_response(event, handle_success, handle_error);
  });
}

void Rest::operator()(Trace<json::Assets> const &event) {
  auto &[trace_info, assets] = event;
  log::info<4>("assets={}"sv, assets);
  // do nothing
}

// asset-pairs

void Rest::get_asset_pairs() {
  profile_.asset_pairs([&]() {
    auto request = web::rest::Request{
        .method = web::http::Method::GET,
        .path = "/0/public/AssetPairs"sv,
        .query = {},
        .accept = web::http::Accept::APPLICATION_JSON,
        .content_type = {},
        .headers = {},
        .body = {},
        .quality_of_service = {},
    };
    auto callback = [this, sequence = download_.sequence()]([[maybe_unused]] auto &request_id, auto &response) {
      TraceInfo trace_info;
      Trace event{trace_info, response};
      get_asset_pairs_ack(event, sequence);
    };
    (*connection_)("asset_pairs"sv, request, callback);
  });
}

void Rest::get_asset_pairs_ack(Trace<web::rest::Response> const &event, uint32_t sequence) {
  constexpr auto const STATE = RestState::ASSET_PAIRS;
  profile_.asset_pairs_ack([&]() {
    auto handle_success = [&](auto &body) {
      if (download_.skip(sequence, STATE)) {
        log::info("Download state={} has already been processed"sv, STATE);
      } else {
        json::AssetPairs asset_pairs{body, decode_buffer_};
        Trace event_2{event, asset_pairs};
        (*this)(event_2);
        download_.check(STATE);
      }
    };
    auto handle_error = [&]([[maybe_unused]] auto origin, [[maybe_unused]] auto status, auto error, auto text) {
      log::warn(R"(error={}, text="{}")"sv, error, text);
      download_.retry(STATE);
    };
    process_response(event, handle_success, handle_error);
  });
}

void Rest::operator()(Trace<json::AssetPairs> const &event) {
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
    auto discard = shared_.discard_symbol(symbol);
    auto tick_size = item.tick_size;
    if (std::isnan(tick_size))
      tick_size = std::pow(double{10.0}, -static_cast<double>(item.pair_decimals));
    auto min_trade_vol = std::pow(double{10.0}, -static_cast<double>(item.lot_decimals));
    auto reference_data = ReferenceData{
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
        .min_notional = NaN,
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
        .discard = discard,
    };
    create_trace_and_dispatch(handler_, trace_info, reference_data, true);
    if (discard)
      continue;
    if (all_symbols_.emplace(symbol).second)  // only include new
      symbols.emplace_back(symbol);
    ++counter;
    auto market_status = MarketStatus{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .trading_status = TradingStatus::OPEN,  // XXX doesn't exist?
    };
    create_trace_and_dispatch(handler_, trace_info, market_status, true);
  }
  log::info("AssetPairs {} / {}"sv, counter, std::size(asset_pairs.result));
  if (!std::empty(symbols)) {
    auto symbols_update = SymbolsUpdate{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

template <typename SuccessHandler, typename ErrorHandler>
void Rest::process_response(
    web::rest::Response const &response, SuccessHandler success_handler, ErrorHandler error_handler) {
  try {
    auto [status, category, body] = response.result();
    log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
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
        error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, text);
        break;
      }
      default:
        response.expect(web::http::Status::OK);  // throws
    }
  } catch (NetworkError &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::GATEWAY, e.request_status(), e.error(), e.what());
  } catch (std::exception &e) {
    log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
    error_handler(Origin::EXCHANGE, RequestStatus::ERROR, Error::UNKNOWN, e.what());
  }
}

}  // namespace kraken
}  // namespace roq

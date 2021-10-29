/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/order_entry.h"

#include <utility>

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/json/parser.h"

#include "roq/core/metrics/factory.h"

#include "roq/kraken/flags.h"

#include "roq/kraken/json/result.h"

using namespace std::literals;

namespace roq {
namespace kraken {

namespace {
static const auto NAME = "om"sv;

static const auto SUPPORTS = utils::Mask{
    SupportType::CREATE_ORDER,
    SupportType::CANCEL_ORDER,
    SupportType::ORDER_ACK,
};
static const auto SUPPORTS_MASTER = utils::Mask{
    SUPPORTS,
    SupportType::REFERENCE_DATA,
    SupportType::MARKET_STATUS,
};

static const auto ALLOW_PIPELINING = true;

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &group, const std::string_view &function)
      : core::metrics::Factory(server::Flags::name(), group, function) {}
};
}  // namespace

OrderEntry::OrderEntry(
    Handler &handler,
    core::io::Context &context,
    uint16_t stream_id,
    Security &security,
    Shared &shared,
    bool master)
    : handler_(handler), stream_id_(stream_id),
      name_(fmt::format("{}:{}:{}"sv, stream_id_, NAME, security.get_account())), master_(master),
      connection_(
          *this,
          context,
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          core::URI(Flags::rest_uri()),
          ROQ_PACKAGE_NAME,
          core::http::Connection::KEEP_ALIVE,
          ALLOW_PIPELINING,
          Flags::rest_request_timeout(),
          Flags::rest_rate_limit_interval(),
          Flags::rest_rate_limit_max_requests(),
          Flags::rest_ping_freq(),
          Flags::rest_ping_path()),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics(name_, "disconnect"sv),
      },
      profile_{
          .assets = create_metrics(name_, "assets"sv),
          .assets_ack = create_metrics(name_, "assets_ack"sv),
          .asset_pairs = create_metrics(name_, "asset_pairs"sv),
          .asset_pairs_ack = create_metrics(name_, "asset_pairs_ack"sv),
          .positions = create_metrics(name_, "positions"sv),
          .positions_ack = create_metrics(name_, "positions_ack"sv),
          .get_web_sockets_token = create_metrics(name_, "get_web_sockets_token"sv),
          .get_web_sockets_token_ack = create_metrics(name_, "get_web_sockets_token_ack"sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"sv),
      },
      security_(security), shared_(shared),
      download_(Flags::rest_request_timeout(), [this](auto state) { return download(state); }) {
}

void OrderEntry::operator()(const Event<Start> &) {
  connection_.start();
}

void OrderEntry::operator()(const Event<Stop> &) {
  connection_.stop();
}

void OrderEntry::operator()(const Event<Timer> &event) {
  connection_.refresh(event.value.now);
}

void OrderEntry::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(counter_.disconnect, metrics::COUNTER)
      // profile
      .write(profile_.assets, metrics::PROFILE)
      .write(profile_.assets_ack, metrics::PROFILE)
      .write(profile_.asset_pairs, metrics::PROFILE)
      .write(profile_.asset_pairs_ack, metrics::PROFILE)
      .write(profile_.positions, metrics::PROFILE)
      .write(profile_.positions_ack, metrics::PROFILE)
      .write(profile_.get_web_sockets_token, metrics::PROFILE)
      .write(profile_.get_web_sockets_token_ack, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

uint16_t OrderEntry::operator()(
    const Event<CreateOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id) {
  throw NotImplementedException();
}

uint16_t OrderEntry::operator()(
    const Event<ModifyOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  throw NotImplementedException();
}

uint16_t OrderEntry::operator()(
    const Event<CancelOrder> &,
    const oms::Order &,
    [[maybe_unused]] const std::string_view &request_id,
    [[maybe_unused]] const std::string_view &previous_request_id) {
  throw NotImplementedException();
}

uint16_t OrderEntry::operator()(
    const Event<CancelAllOrders> &, [[maybe_unused]] const std::string_view &request_id) {
  log::warn("*** CANCEL ALL ORDERS *NOT* SUPPORTED ***"sv);
  return stream_id_;
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    auto trace_info = server::create_trace_info();
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = (master_ ? SUPPORTS_MASTER : SUPPORTS).get(),
        .status = status_,
        .type = StreamType::REST,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"sv, stream_status);
    server::create_trace_and_dispatch(handler_, trace_info, stream_status);
  }
}

void OrderEntry::operator()(const core::web::Client::Connected &) {
  if (download_.downloading()) {
    download_.bump();
  } else {
    (*this)(ConnectionStatus::DOWNLOADING);
    download_.begin();
  }
}

void OrderEntry::operator()(const core::web::Client::Disconnected &) {
  ++counter_.disconnect;
  (*this)(ConnectionStatus::DISCONNECTED);
  if (!download_.downloading())
    download_.reset();
}

void OrderEntry::operator()(const core::web::Client::Latency &latency) {
  auto trace_info = server::create_trace_info();
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(handler_, trace_info, external_latency);
  latency_.ping.update(latency.sample);
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    case OrderEntryState::UNDEFINED:
      assert(false);
      break;
    case OrderEntryState::TOKEN:
      get_token();
      return 1;
    case OrderEntryState::ASSETS:
      if (master_) {
        get_assets();
        return 1;
      } else {
        return {};
      }
    case OrderEntryState::ASSET_PAIRS:
      if (master_) {
        get_asset_pairs();
        return 1;
      } else {
        return {};
      }
    case OrderEntryState::POSITIONS:
      get_positions();
      return 1;
    case OrderEntryState::DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

// token

void OrderEntry::get_token() {
  profile_.get_web_sockets_token([&]() {
    auto method = core::http::Method::POST;
    auto path = "/0/private/GetWebSocketsToken"sv;
    auto body = security_.create_body();
    auto headers = security_.create_headers(method, path, body);
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = core::http::ContentType::FORM,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
        .rate_limit_weight = 1,
    };
    auto sequence = download_.sequence();
    connection_(
        "token"sv, request, [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          auto trace_info = server::create_trace_info();
          server::Trace event(trace_info, response);
          get_token_ack(event, sequence);
        });
  });
}

void OrderEntry::get_token_ack(const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.get_web_sockets_token([&]() {
    // auto &[trace_info, response] = event;
    auto &trace_info = event.trace_info;
    auto &response = event.value;
    auto state = OrderEntryState::TOKEN;
    try {
      auto [status, category, body] = response.result();
      log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
      if (download_.skip(sequence, state)) {
        log::info("Download state={} has already been processed"sv, state);
        return;
      }
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      json::Result::dispatch<json::Token>(
          body,
          buffer,
          [](const roq::span<std::string_view> &e) {  // error
            log::warn("error=[{}]"sv, fmt::join(e, ","sv));
            log::fatal("Unexpected"sv);
          },
          [&](const json::Token &token) {  // success
            server::Trace event(trace_info, token);
            (*this)(event);
          });
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::Token> &event) {
  auto &[trace_info, token] = event;
  log::info<2>(R"(token={})"sv, token);
  TokenUpdate token_update{
      .account = security_.get_account(),
      .token = token.token,
  };
  handler_(token_update);
}

// assets

void OrderEntry::get_assets() {
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
        .rate_limit_weight = 1,
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

void OrderEntry::get_assets_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.assets_ack([&]() {
    auto &[trace_info, response] = event;
    auto state = OrderEntryState::ASSETS;
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

void OrderEntry::operator()(const server::Trace<json::Assets> &event) {
  auto &[trace_info, assets] = event;
  log::info<4>("assets={}"sv, assets);
  // do nothing
}

// asset-pairs

void OrderEntry::get_asset_pairs() {
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
        .rate_limit_weight = 1,
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

void OrderEntry::get_asset_pairs_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.asset_pairs_ack([&]() {
    auto &[trace_info, response] = event;
    auto state = OrderEntryState::ASSET_PAIRS;
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

void OrderEntry::operator()(const server::Trace<json::AssetPairs> &event) {
  auto &[trace_info, asset_pairs] = event;
  log::info<4>("asset_pairs={}"sv, asset_pairs);
  assert(asset_pairs.error.empty());
  std::vector<std::string> symbols;
  symbols.reserve(asset_pairs.result.size());
  size_t counter = {};
  for (auto &item : asset_pairs.result) {
    log::info<2>("item={}"sv, item);
    if (item.wsname.empty()) {
      log::info<1>(R"(Skipping altname="{}", reason: wsname is empty)"sv, item.altname);
      continue;
    }
    std::string symbol(item.wsname);
    // remove escape
    symbol.erase(std::remove(symbol.begin(), symbol.end(), '\\'), symbol.end());
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
  log::info("AssetPairs {} / {}"sv, counter, asset_pairs.result.size());
  if (!symbols.empty()) {
    SymbolsUpdate symbols_update{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

// positions

void OrderEntry::get_positions() {
  profile_.positions([&]() {
    auto method = core::http::Method::POST;
    auto path = "/0/private/OpenPositions"sv;
    auto body = security_.create_body();
    auto headers = security_.create_headers(method, path, body);
    core::web::Request request{
        .method = method,
        .path = path,
        .query = {},
        .accept = core::http::Accept::JSON,
        .content_type = core::http::ContentType::FORM,
        .headers = headers,
        .body = body,
        .quality_of_service = {},
        .rate_limit_weight = 1,
    };
    auto sequence = download_.sequence();
    connection_(
        "positions"sv,
        request,
        [this, sequence]([[maybe_unused]] auto &request_id, auto &response) {
          auto trace_info = server::create_trace_info();
          server::Trace event(trace_info, response);
          get_positions_ack(event, sequence);
        });
  });
}

void OrderEntry::get_positions_ack(
    const server::Trace<core::web::Response> &event, uint32_t sequence) {
  profile_.positions_ack([&]() {
    auto &[trace_info, response] = event;
    auto state = OrderEntryState::POSITIONS;
    try {
      auto [status, category, body] = response.result();
      log::debug(R"(status={}, category={}, body="{}")"sv, status, category, body);
      if (download_.skip(sequence, state)) {
        log::info("Download state={} has already been processed"sv, state);
        return;
      }
      response.expect(core::http::Status::OK);
      core::json::Buffer buffer(decode_buffer_);
      auto positions = core::json::Parser::create<json::Positions>(body, buffer);
      server::Trace event(trace_info, positions);
      (*this)(event);
      download_.check(state);
    } catch (core::NetworkError &e) {
      log::warn(R"(Exception type={}, what="{}")"sv, typeid(e).name(), e.what());
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const server::Trace<json::Positions> &event) {
  auto &[trace_info, positions] = event;
  log::info<4>("positions={}"sv, positions);
  assert(positions.error.empty());
}

}  // namespace kraken
}  // namespace roq

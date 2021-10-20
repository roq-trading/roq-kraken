/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/order_entry.h"

#include <utility>

#include "roq/utils/mask.h"
#include "roq/utils/update.h"

#include "roq/core/json/parser.h"

#include "roq/core/metrics/factory.h"

#include "roq/kraken/flags.h"

#include "roq/kraken/json/result.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

namespace {
static const auto NAME = "om"_sv;

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
      name_(fmt::format("{}:{}:{}"_sv, stream_id_, NAME, security.get_account())), master_(master),
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
          .disconnect = create_metrics(name_, "disconnect"_sv),
      },
      profile_{
          .assets = create_metrics(name_, "assets"_sv),
          .asset_pairs = create_metrics(name_, "asset_pairs"_sv),
          .balance = create_metrics(name_, "balance"_sv),
          .open_positions = create_metrics(name_, "open_positions"_sv),
          .get_web_sockets_token = create_metrics(name_, "get_web_sockets_token"_sv),
      },
      latency_{
          .ping = create_metrics(name_, "ping"_sv),
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
      .write(profile_.asset_pairs, metrics::PROFILE)
      .write(profile_.balance, metrics::PROFILE)
      .write(profile_.open_positions, metrics::PROFILE)
      .write(profile_.get_web_sockets_token, metrics::PROFILE)
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
  log::warn("*** CANCEL ALL ORDERS *NOT* SUPPORTED ***"_sv);
  return stream_id_;
}

void OrderEntry::operator()(ConnectionStatus status) {
  if (utils::update(status_, status)) {
    server::TraceInfo trace_info;
    StreamStatus stream_status{
        .stream_id = stream_id_,
        .account = security_.get_account(),
        .supports = (master_ ? SUPPORTS_MASTER : SUPPORTS).get(),
        .status = status_,
        .type = StreamType::REST,
        .priority = Priority::PRIMARY,
    };
    log::info("stream_status={}"_sv, stream_status);
    server::create_trace_and_dispatch(trace_info, stream_status, handler_);
  }
}

template <>
void OrderEntry::get(std::function<void(const core::Promise<json::Assets> &)> &&callback) {
  core::web::Request request{
      .method = core::http::Method::GET,
      .path = "/0/public/Assets"_sv,
      .query = {},
      .accept = core::http::Accept::JSON,
      .content_type = {},
      .headers = {},
      .body = {},
      .quality_of_service = {},
      .rate_limit_weight = 1,
  };
  connection_(
      "assets"_sv,
      request,
      [this, callback{std::move(callback)}]([[maybe_unused]] auto &request_id, auto &response) {
        profile_.assets([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto assets = core::json::Parser::create<json::Assets>(response.body(), buffer);
            if (assets.error.empty()) {
              log::info<1>("assets={}"_sv, assets);
              core::Promise<json::Assets> promise(assets);
              callback(promise);
            } else {
              log::warn("assets={}"_sv, assets);
              log::fatal("Unexpected"_sv);
            }
          } catch (core::NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
            core::Promise<json::Assets> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void OrderEntry::get(std::function<void(const core::Promise<json::AssetPairs> &)> &&callback) {
  core::web::Request request{
      .method = core::http::Method::GET,
      .path = "/0/public/AssetPairs"_sv,
      .query = {},
      .accept = core::http::Accept::JSON,
      .content_type = {},
      .headers = {},
      .body = {},
      .quality_of_service = {},
      .rate_limit_weight = 1,
  };
  connection_(
      "asset_pairs"_sv,
      request,
      [this, callback{std::move(callback)}]([[maybe_unused]] auto &request_id, auto &response) {
        profile_.asset_pairs([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto asset_pairs =
                core::json::Parser::create<json::AssetPairs>(response.body(), buffer);
            if (asset_pairs.error.empty()) {
              log::info<1>("asset_pairs={}"_sv, asset_pairs);
              core::Promise<json::AssetPairs> promise(asset_pairs);
              callback(promise);
            } else {
              log::warn("asset_pairs={}"_sv, asset_pairs);
              log::fatal("Unexpected"_sv);
            }
          } catch (core::NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
            core::Promise<json::AssetPairs> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

/*
template <>
void OrderEntry::get(
    std::function<void(const core::Promise<json::Balance>&)>&& callback) {
  auto method = core::http::Method::POST;
  auto path = "/0/private/Balance"_sv;
  auto body = security_.create_body();
  auto headers = security_.create_headers(
      method,
      path,
      body);
  auto rate_limit_weight = 1;
  connection_.request(
      method,
      path,
      {},  // query
      headers,
      body,
      {},  // QoS
      rate_limit_weight,
      [this, callback{std::move(callback)}](auto& response) {
    profile_.balance(
        [&]() {
      try {
        response.expect(core::http::Status::OK);
        core::json::Buffer buffer(decode_buffer_);
        auto balance =
          core::json::Parser::create<json::Balance>(
              response.body(),
              buffer);
        if (balance.error.empty()) {
          log::info<1>(
              "balance={}"_sv,
              balance);
          handler_(balance);
        } else {
          log::warn(
              "balance={}"_sv,
              balance);
          log::fatal("Unexpected"_sv);
        }
      } catch (core::NetworkError& e) {
        log::warn(
            R"(Exception type={}, what="{}")"_sv,
            typeid(e).name(),
            e.what());
        core::Promise<json::Products> promise(std::current_exception());
        callback(promise);
      }
    });
  });
}
*/

template <>
void OrderEntry::get(std::function<void(const core::Promise<json::Positions> &)> &&callback) {
  auto method = core::http::Method::POST;
  auto path = "/0/private/OpenPositions"_sv;
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
  connection_(
      "positions"_sv,
      request,
      [this, callback{std::move(callback)}]([[maybe_unused]] auto &request_id, auto &response) {
        profile_.open_positions([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto positions = core::json::Parser::create<json::Positions>(response.body(), buffer);
            if (positions.error.empty()) {
              log::info<1>("positions={}"_sv, positions);
              core::Promise<json::Positions> promise(positions);
              callback(promise);
            } else {
              log::warn("positions={}"_sv, positions);
              log::fatal("Unexpected"_sv);
            }
          } catch (core::NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
            core::Promise<json::Positions> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void OrderEntry::get(std::function<void(const core::Promise<json::Token> &)> &&callback) {
  auto method = core::http::Method::POST;
  auto path = "/0/private/GetWebSocketsToken"_sv;
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
  connection_(
      "token"_sv,
      request,
      [this, callback{std::move(callback)}]([[maybe_unused]] auto &request_id, auto &response) {
        profile_.get_web_sockets_token([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            json::Result::dispatch<json::Token>(
                response.body(),
                buffer,
                [](const roq::span<std::string_view> &e) {
                  log::warn("error=[{}]"_sv, fmt::join(e, ","_sv));
                  log::fatal("Unexpected"_sv);
                },
                [&](const json::Token &token) {
                  log::info<1>("token={}"_sv, token);
                  core::Promise<json::Token> promise(token);
                  callback(promise);
                });
          } catch (core::NetworkError &e) {
            log::warn(R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
            core::Promise<json::Token> promise(std::current_exception());
            callback(promise);
          }
        });
      });
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
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .stream_id = stream_id_,
      .latency = latency.sample,
  };
  server::create_trace_and_dispatch(trace_info, external_latency, handler_);
  latency_.ping.update(latency.sample);
}

uint32_t OrderEntry::download(OrderEntryState state) {
  switch (state) {
    case OrderEntryState::UNDEFINED:
      assert(false);
      break;
    case OrderEntryState::TOKEN:
      download_token();
      return 1;
    case OrderEntryState::ASSETS:
      if (master_) {
        download_assets();
        return 1;
      } else {
        return {};
      }
    case OrderEntryState::ASSET_PAIRS:
      if (master_) {
        download_asset_pairs();
        return 1;
      } else {
        return {};
      }
    case OrderEntryState::BALANCE:
      download_balance();
      // return 1;
      return {};
    case OrderEntryState::OPEN_POSITIONS:
      download_open_positions();
      return 1;
    case OrderEntryState::DONE:
      (*this)(ConnectionStatus::READY);
      return {};
  }
  assert(false);
  return {};
}

void OrderEntry::download_token() {
  constexpr auto state = OrderEntryState::TOKEN;
  auto sequence = download_.sequence();
  get<json::Token>([this, sequence](auto &promise) {
    try {
      if (download_.skip(sequence, state))
        return;
      (*this)(promise.get());
      download_.check(state);
    } catch (core::NetworkError &) {
      download_.retry(state);
    }
  });
}
void OrderEntry::download_assets() {
  constexpr auto state = OrderEntryState::ASSETS;
  auto sequence = download_.sequence();
  get<json::Assets>([this, sequence](auto &promise) {
    try {
      if (download_.skip(sequence, state))
        return;
      (*this)(promise.get());
      download_.check(state);
    } catch (core::NetworkError &) {
      download_.retry(state);
    }
  });
}

void OrderEntry::download_asset_pairs() {
  constexpr auto state = OrderEntryState::ASSET_PAIRS;
  auto sequence = download_.sequence();
  get<json::AssetPairs>([this, sequence](auto &promise) {
    try {
      if (download_.skip(sequence, state))
        return;
      (*this)(promise.get());
      download_.check(state);
    } catch (core::NetworkError &) {
      download_.retry(state);
    }
  });
}

void OrderEntry::download_balance() {
  constexpr auto state = OrderEntryState::BALANCE;
  std::ignore = state;
  /*
  auto sequence = download_.sequence();
  get<json::Balance>(
      [this, sequence](auto& promise) {
    try {
      if (download_.skip(sequence, state)) return;
      (*this)(promise.get());
      download_.check(state);
    } catch (core::NetworkError&) {
      download_.retry(state);
    }
  });
  */
}

void OrderEntry::download_open_positions() {
  constexpr auto state = OrderEntryState::OPEN_POSITIONS;
  auto sequence = download_.sequence();
  get<json::Positions>([this, sequence](auto &promise) {
    try {
      if (download_.skip(sequence, state))
        return;
      (*this)(promise.get());
      download_.check(state);
    } catch (core::NetworkError &) {
      download_.retry(state);
    }
  });
}

void OrderEntry::operator()(const json::Token &token) {
  log::info(R"(token={})"_sv, token);
  TokenUpdate token_update{
      .account = security_.get_account(),
      .token = token.token,
  };
  handler_(token_update);
}

void OrderEntry::operator()(const json::Assets &) {
}

void OrderEntry::operator()(const json::AssetPairs &asset_pairs) {
  assert(asset_pairs.error.empty());
  server::TraceInfo trace_info;  // XXX not correct (*parsing* already done)
  std::vector<std::string> symbols;
  symbols.reserve(asset_pairs.result.size());
  size_t counter = {};
  for (auto &item : asset_pairs.result) {
    log::info<1>("item={}"_sv, item);
    if (item.wsname.empty()) {
      log::info<1>(R"(Skipping altname="{}", reason: wsname is empty)"_sv, item.altname);
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
    server::create_trace_and_dispatch(trace_info, reference_data, handler_, true);
    MarketStatus market_status{
        .stream_id = stream_id_,
        .exchange = Flags::exchange(),
        .symbol = symbol,
        .trading_status = TradingStatus::OPEN,  // XXX doesn't exist?
    };
    server::create_trace_and_dispatch(trace_info, market_status, handler_, true);
  }
  log::info("AssetPairs {} / {}"_sv, counter, asset_pairs.result.size());
  if (!symbols.empty()) {
    SymbolsUpdate symbols_update{
        .symbols = symbols,
    };
    handler_(symbols_update);
  }
}

void OrderEntry::operator()(const json::Positions &positions) {
  assert(positions.error.empty());
}

}  // namespace kraken
}  // namespace roq

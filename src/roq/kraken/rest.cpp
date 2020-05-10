/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/rest.h"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include "roq/core/json/parser.h"

#include "roq/kraken/gateway.h"
#include "roq/kraken/options.h"

#include "roq/kraken/json/result.h"
#include "roq/kraken/json/utils.h"

#include "roq/kraken/json/assets.h"
#include "roq/kraken/json/asset_pairs.h"
#include "roq/kraken/json/positions.h"
#include "roq/kraken/json/token.h"

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "rest";

static auto create_counter(
    const std::string_view& function) {
  return core::metrics::Counter(
      FLAGS_name,
      CONNECTION,
      function);
}

static auto create_profile(
    const std::string_view& function) {
  return core::metrics::Profile(
      FLAGS_name,
      CONNECTION,
      function);
}

static auto create_latency(
    const std::string_view& function) {
  return core::metrics::Latency(
      FLAGS_name,
      CONNECTION,
      function);
}
}  // namespace

Rest::Rest(
    Gateway& gateway,
    const Config& config,
    Random& random,
    core::event::Base& base,
    core::event::DNSBase& dns_base,
    core::ssl::Context& ssl_context)
    : _gateway(gateway),
      _random(random),
      _connection(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(FLAGS_rest_uri),
          PACKAGE_NAME,
          true,  // keep alive
          std::chrono::seconds { FLAGS_rate_limit_interval_secs },
          FLAGS_rate_limit_max_requests,
          std::chrono::seconds { FLAGS_ping_freq_secs },
          FLAGS_decode_buffer_size,
          FLAGS_encode_buffer_size,
          FLAGS_rest_ping_path),
      _decode_buffer(FLAGS_decode_buffer_size),
      _counter {
        .disconnect = create_counter("disconnect"),
      },
      _profile {
        .assets = create_profile("assets"),
        .asset_pairs = create_profile("asset_pairs"),
        .balance = create_profile("balance"),
        .open_positions = create_profile("open_positions"),
        .get_web_sockets_token = create_profile("get_web_sockets_token"),
      },
      _latency {
        .ping = create_latency("ping"),
      } {
  (void) config;  // avoid warning
}

bool Rest::ready() const {
  return _connection.ready();
}

void Rest::close() {
  _connection.close();
}

void Rest::operator()(const StartEvent&) {
  _connection.start();
}

void Rest::operator()(const StopEvent&) {
  _connection.stop();
}

void Rest::operator()(const TimerEvent& event) {
  _connection.refresh(event.now);
}

void Rest::operator()(Metrics& metrics) {
  metrics
    // counter
    .write(_counter.disconnect)
    // profile
    .write(_profile.assets)
    .write(_profile.asset_pairs)
    .write(_profile.balance)
    .write(_profile.open_positions)
    .write(_profile.get_web_sockets_token)
    // latency
    .write(_latency.ping);
}

void Rest::get_assets(
    std::function<void(const core::web::Response&)>&& callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/0/public/Assets";
  _connection.request(
      method,
      path,
      std::string_view(),  // headers
      std::string_view(),  // body
      [this, callback](auto& response) {
        if (response.success()) {
          auto [status, body] = response.get();
          if (status == core::http::Status::OK) {
            _profile.assets(
                [&]() {
                  core::json::Buffer buffer(_decode_buffer);
                  auto assets =
                    core::json::Parser::create<json::Assets>(
                        body,
                        buffer);
                  if (assets.error.empty()) {
                    VLOG(1)(
                        FMT_STRING(R"(assets={})"),
                        assets);
                    _gateway(assets);
                  } else {
                    LOG(WARNING)(
                        FMT_STRING(R"(assets={})"),
                        assets);
                    LOG(FATAL)("Unexpected");
                  }
                });
          }
        }
        callback(response);
      });
}

void Rest::get_asset_pairs(
    std::function<void(const core::web::Response&)>&& callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/0/public/AssetPairs";
  _connection.request(
      method,
      path,
      std::string_view(),  // headers
      std::string_view(),  // body
      [this, callback](auto& response) {
        if (response.success()) {
          auto [status, body] = response.get();
          if (status == core::http::Status::OK) {
            _profile.asset_pairs(
                [&]() {
                  core::json::Buffer buffer(_decode_buffer);
                  auto asset_pairs =
                    core::json::Parser::create<json::AssetPairs>(
                        body,
                        buffer);
                  if (asset_pairs.error.empty()) {
                    VLOG(1)(
                        FMT_STRING(R"(asset_pairs={})"),
                        asset_pairs);
                    _gateway(asset_pairs);
                  } else {
                    LOG(WARNING)(
                        FMT_STRING(R"(asset_pairs={})"),
                        asset_pairs);
                    LOG(FATAL)("Unexpected");
                  }
                });
          }
        }
        callback(response);
      });
}

void Rest::get_balance(
    std::function<void(const core::web::Response&)>&& callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/0/private/Balance";
  auto body = _random.create_body();
  auto headers = _random.create_headers(
      method,
      path,
      body);
  _connection.request(
      method,
      path,
      headers,
      body,
      [this, callback](auto& response) {
        if (response.success()) {
          auto [status, body] = response.get();
          if (status == core::http::Status::OK) {
            _profile.balance(
                [&]() {
                  /*
                  core::json::Buffer buffer(_decode_buffer);
                  auto balance =
                    core::json::Parser::create<json::Balance>(
                        body,
                        buffer);
                  if (balance.error.empty()) {
                    VLOG(1)(
                        FMT_STRING(R"(balance={})"),
                        balance);
                    _gateway(balance);
                  } else {
                    LOG(WARNING)(
                        FMT_STRING(R"(balance={})"),
                        balance);
                    LOG(FATAL)("Unexpected");
                  }
                  */
                });
          }
        }
        callback(response);
      });
}

void Rest::get_open_positions(
    std::function<void(const core::web::Response&)>&& callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/0/private/OpenPositions";
  auto body = _random.create_body();
  auto headers = _random.create_headers(
      method,
      path,
      body);
  _connection.request(
      method,
      path,
      headers,
      body,
      [this, callback](auto& response) {
        if (response.success()) {
          auto [status, body] = response.get();
          if (status == core::http::Status::OK) {
            _profile.open_positions(
                [&]() {
                  core::json::Buffer buffer(_decode_buffer);
                  auto positions =
                    core::json::Parser::create<json::Positions>(
                        body,
                        buffer);
                  if (positions.error.empty()) {
                    VLOG(1)(
                        FMT_STRING(R"(positions={})"),
                        positions);
                    _gateway(positions);
                  } else {
                    LOG(WARNING)(
                        FMT_STRING(R"(positions={})"),
                        positions);
                    LOG(FATAL)("Unexpected");
                  }
                });
          }
        }
        callback(response);
      });
}

void Rest::get_web_sockets_token(
    std::function<void(const core::web::Response&)>&& callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/0/private/GetWebSocketsToken";
  auto body = _random.create_body();
  auto headers = _random.create_headers(
      method,
      path,
      body);
  _connection.request(
      method,
      path,
      headers,
      body,
      [this, callback](auto& response) {
        if (response.success()) {
          auto [status, body] = response.get();
          if (status == core::http::Status::OK) {
            _profile.get_web_sockets_token(
                [&]() {
                  core::json::Buffer buffer(_decode_buffer);
                  json::Result::dispatch<json::Token>(
                      body,
                      buffer,
                      [](const roq::span<std::string_view>& e) {
                        LOG(WARNING)(
                            FMT_STRING(R"(error=[{}])"),
                            fmt::join(e, ","));
                        LOG(FATAL)("Unexpected");
                      },
                      [&](const json::Token& token) {
                        VLOG(1)(
                            FMT_STRING(R"(token={})"),
                            token);
                        _gateway(token);
                      });
                });
          }
        }
        callback(response);
      });
}

void Rest::operator()(const core::web::Client::Connected&) {
  _gateway(*this);
}

void Rest::operator()(const core::web::Client::Disconnected&) {
  ++_counter.disconnect;
  _gateway(*this);
}

void Rest::operator()(const core::web::Client::Latency& latency) {
  _latency.ping.update(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          latency.sample).count());
}

}  // namespace kraken
}  // namespace roq

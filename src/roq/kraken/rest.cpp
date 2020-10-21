/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/rest.h"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "roq/core/json/parser.h"

#include "roq/kraken/options.h"

#include "roq/kraken/json/result.h"
#include "roq/kraken/json/utils.h"

#include "roq/kraken/json/asset_pairs.h"
#include "roq/kraken/json/assets.h"
#include "roq/kraken/json/positions.h"
#include "roq/kraken/json/token.h"

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "rest";

static auto create_counter(const std::string_view &function) {
  return core::metrics::Counter(FLAGS_name, CONNECTION, function);
}

static auto create_profile(const std::string_view &function) {
  return core::metrics::Profile(FLAGS_name, CONNECTION, function);
}

static auto create_latency(const std::string_view &function) {
  return core::metrics::Latency(FLAGS_name, CONNECTION, function);
}
}  // namespace

Rest::Rest(
    Handler &handler,
    [[maybe_unused]] const Config &config,
    Random &random,
    core::event::Base &base,
    core::event::DNSBase &dns_base,
    core::ssl::Context &ssl_context)
    : _handler(handler), _random(random),
      _connection(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(FLAGS_rest_uri),
          ROQ_PACKAGE_NAME,
          true,  // keep alive
          FLAGS_rest_request_queue_depth,
          std::chrono::seconds{FLAGS_rest_request_timeout_secs},
          std::chrono::seconds{FLAGS_rest_rate_limit_interval_secs},
          FLAGS_rest_rate_limit_max_requests,
          std::chrono::seconds{FLAGS_rest_ping_freq_secs},
          FLAGS_decode_buffer_size,
          FLAGS_encode_buffer_size,
          FLAGS_rest_ping_path),
      _decode_buffer(FLAGS_decode_buffer_size),
      _counter{
          .disconnect = create_counter("disconnect"),
      },
      _profile{
          .assets = create_profile("assets"),
          .asset_pairs = create_profile("asset_pairs"),
          .balance = create_profile("balance"),
          .open_positions = create_profile("open_positions"),
          .get_web_sockets_token = create_profile("get_web_sockets_token"),
      },
      _latency{
          .ping = create_latency("ping"),
      } {
}

bool Rest::ready() const {
  return _connection.ready();
}

void Rest::operator()(const Event<Start> &) {
  _connection.start();
}

void Rest::operator()(const Event<Stop> &) {
  _connection.stop();
}

void Rest::operator()(const Event<Timer> &event) {
  _connection.refresh(event.value.now);
}

void Rest::operator()(metrics::Writer &writer) {
  writer
      // counter
      .write(_counter.disconnect, metrics::COUNTER)
      // profile
      .write(_profile.assets, metrics::PROFILE)
      .write(_profile.asset_pairs, metrics::PROFILE)
      .write(_profile.balance, metrics::PROFILE)
      .write(_profile.open_positions, metrics::PROFILE)
      .write(_profile.get_web_sockets_token, metrics::PROFILE)
      // latency
      .write(_latency.ping, metrics::LATENCY);
}

template <>
void Rest::get(
    std::function<void(const core::Promise<json::Assets> &)> &&callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/0/public/Assets";
  _connection.request(
      method,
      path,
      std::string_view(),  // query
      std::string_view(),  // headers
      std::string_view(),  // body
      [this, callback](auto &response) {
        _profile.assets([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(_decode_buffer);
            auto assets = core::json::Parser::create<json::Assets>(
                response.body(), buffer);
            if (assets.error.empty()) {
              VLOG(1)(R"(assets={})", assets);
              core::Promise<json::Assets> promise(assets);
              callback(promise);
            } else {
              LOG(WARNING)(R"(assets={})", assets);
              LOG(FATAL)("Unexpected");
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")", typeid(e).name(), e.what());
            core::Promise<json::Assets> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void Rest::get(
    std::function<void(const core::Promise<json::AssetPairs> &)> &&callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/0/public/AssetPairs";
  _connection.request(
      method,
      path,
      std::string_view(),  // query
      std::string_view(),  // headers
      std::string_view(),  // body
      [this, callback](auto &response) {
        _profile.asset_pairs([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(_decode_buffer);
            auto asset_pairs = core::json::Parser::create<json::AssetPairs>(
                response.body(), buffer);
            if (asset_pairs.error.empty()) {
              VLOG(1)(R"(asset_pairs={})", asset_pairs);
              core::Promise<json::AssetPairs> promise(asset_pairs);
              callback(promise);
            } else {
              LOG(WARNING)(R"(asset_pairs={})", asset_pairs);
              LOG(FATAL)("Unexpected");
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")", typeid(e).name(), e.what());
            core::Promise<json::AssetPairs> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

/*
template <>
void Rest::get(
    std::function<void(const core::Promise<json::Balance>&)>&& callback) {
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
      std::string_view(),  // query
      headers,
      body,
      [this, callback](auto& response) {
    _profile.balance(
        [&]() {
      try {
        response.expect(core::http::Status::OK);
        core::json::Buffer buffer(_decode_buffer);
        auto balance =
          core::json::Parser::create<json::Balance>(
              response.body(),
              buffer);
        if (balance.error.empty()) {
          VLOG(1)(
              R"(balance={})",
              balance);
          _handler(balance);
        } else {
          LOG(WARNING)(
              R"(balance={})",
              balance);
          LOG(FATAL)("Unexpected");
        }
      } catch (NetworkError& e) {
        LOG(WARNING)(
            R"(Exception type={}, what="{}")",
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
void Rest::get(
    std::function<void(const core::Promise<json::Positions> &)> &&callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/0/private/OpenPositions";
  auto body = _random.create_body();
  auto headers = _random.create_headers(method, path, body);
  _connection.request(
      method,
      path,
      std::string_view(),  // query
      headers,
      body,
      [this, callback](auto &response) {
        _profile.open_positions([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(_decode_buffer);
            auto positions = core::json::Parser::create<json::Positions>(
                response.body(), buffer);
            if (positions.error.empty()) {
              VLOG(1)(R"(positions={})", positions);
              core::Promise<json::Positions> promise(positions);
              callback(promise);
            } else {
              LOG(WARNING)(R"(positions={})", positions);
              LOG(FATAL)("Unexpected");
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")", typeid(e).name(), e.what());
            core::Promise<json::Positions> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void Rest::get(
    std::function<void(const core::Promise<json::Token> &)> &&callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/0/private/GetWebSocketsToken";
  auto body = _random.create_body();
  auto headers = _random.create_headers(method, path, body);
  _connection.request(
      method,
      path,
      std::string_view(),  // query
      headers,
      body,
      [this, callback](auto &response) {
        _profile.get_web_sockets_token([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(_decode_buffer);
            json::Result::dispatch<json::Token>(
                response.body(),
                buffer,
                [](const roq::span<std::string_view> &e) {
                  LOG(WARNING)(R"(error=[{}])", fmt::join(e, ","));
                  LOG(FATAL)("Unexpected");
                },
                [&](const json::Token &token) {
                  VLOG(1)(R"(token={})", token);
                  core::Promise<json::Token> promise(token);
                  callback(promise);
                });
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")", typeid(e).name(), e.what());
            core::Promise<json::Token> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

void Rest::operator()(const core::web::Client::Connected &) {
  _handler(*this);
}

void Rest::operator()(const core::web::Client::Disconnected &) {
  ++_counter.disconnect;
  _handler(*this);
}

void Rest::operator()(const core::web::Client::Latency &latency) {
  _latency.ping.update(
      std::chrono::duration_cast<std::chrono::nanoseconds>(latency.sample)
          .count());
}

}  // namespace kraken
}  // namespace roq

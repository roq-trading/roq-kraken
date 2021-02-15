/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/rest.h"

#include <fmt/chrono.h>

#include <utility>

#include "roq/core/json/parser.h"

#include "roq/kraken/flags.h"

#include "roq/kraken/json/result.h"
#include "roq/kraken/json/utils.h"

#include "roq/kraken/json/asset_pairs.h"
#include "roq/kraken/json/assets.h"
#include "roq/kraken/json/positions.h"
#include "roq/kraken/json/token.h"

using namespace roq::literals;

namespace roq {
namespace kraken {

namespace {
constexpr std::string_view CONNECTION = "rest"_sv;

static const std::string_view ACCEPT_JSON = "application/json"_sv;
static const std::string_view CONTENT_TYPE_FORM = "application/x-www-form-urlencoded"_sv;

static auto create_counter(const std::string_view &function) {
  return core::metrics::Counter(Flags::name(), CONNECTION, function);
}

static auto create_profile(const std::string_view &function) {
  return core::metrics::Profile(Flags::name(), CONNECTION, function);
}

static auto create_latency(const std::string_view &function) {
  return core::metrics::Latency(Flags::name(), CONNECTION, function);
}
}  // namespace

Rest::Rest(
    Handler &handler,
    [[maybe_unused]] const Config &config,
    Random &random,
    core::event::Base &base,
    core::event::DNSBase &dns_base,
    core::ssl::Context &ssl_context)
    : handler_(handler), random_(random),
      connection_(
          *this,
          base,
          dns_base,
          ssl_context,
          core::URI(Flags::rest_uri()),
          ROQ_PACKAGE_NAME,
          true,  // keep alive
          Flags::rest_request_queue_depth(),
          std::chrono::seconds{Flags::rest_request_timeout_secs()},
          std::chrono::seconds{Flags::rest_rate_limit_interval_secs()},
          Flags::rest_rate_limit_max_requests(),
          std::chrono::seconds{Flags::rest_ping_freq_secs()},
          Flags::decode_buffer_size(),
          Flags::encode_buffer_size(),
          Flags::rest_ping_path()),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_counter("disconnect"_sv),
      },
      profile_{
          .assets = create_profile("assets"_sv),
          .asset_pairs = create_profile("asset_pairs"_sv),
          .balance = create_profile("balance"_sv),
          .open_positions = create_profile("open_positions"_sv),
          .get_web_sockets_token = create_profile("get_web_sockets_token"_sv),
      },
      latency_{
          .ping = create_latency("ping"_sv),
      } {
}

bool Rest::ready() const {
  return connection_.ready();
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
      .write(profile_.asset_pairs, metrics::PROFILE)
      .write(profile_.balance, metrics::PROFILE)
      .write(profile_.open_positions, metrics::PROFILE)
      .write(profile_.get_web_sockets_token, metrics::PROFILE)
      // latency
      .write(latency_.ping, metrics::LATENCY);
}

template <>
void Rest::get(std::function<void(const core::Promise<json::Assets> &)> &&callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/0/public/Assets"_sv;
  connection_.request(
      method,
      path,
      std::string_view(),  // query
      ACCEPT_JSON,
      std::string_view(),  // content_type
      std::string_view(),  // headers
      std::string_view(),  // body
      [this, callback{std::move(callback)}](auto &response) {
        profile_.assets([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto assets = core::json::Parser::create<json::Assets>(response.body(), buffer);
            if (assets.error.empty()) {
              VLOG(1)(R"(assets={})"_sv, assets);
              core::Promise<json::Assets> promise(assets);
              callback(promise);
            } else {
              LOG(WARNING)(R"(assets={})"_sv, assets);
              LOG(FATAL)("Unexpected"_sv);
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
            core::Promise<json::Assets> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void Rest::get(std::function<void(const core::Promise<json::AssetPairs> &)> &&callback) {
  constexpr auto method = core::http::Method::GET;
  constexpr std::string_view path = "/0/public/AssetPairs"_sv;
  connection_.request(
      method,
      path,
      std::string_view(),  // query
      ACCEPT_JSON,
      std::string_view(),  // content_type
      std::string_view(),  // headers
      std::string_view(),  // body
      [this, callback{std::move(callback)}](auto &response) {
        profile_.asset_pairs([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto asset_pairs =
                core::json::Parser::create<json::AssetPairs>(response.body(), buffer);
            if (asset_pairs.error.empty()) {
              VLOG(1)(R"(asset_pairs={})"_sv, asset_pairs);
              core::Promise<json::AssetPairs> promise(asset_pairs);
              callback(promise);
            } else {
              LOG(WARNING)(R"(asset_pairs={})"_sv, asset_pairs);
              LOG(FATAL)("Unexpected"_sv);
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
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
  constexpr std::string_view path = "/0/private/Balance"_sv;
  auto body = random_.create_body();
  auto headers = random_.create_headers(
      method,
      path,
      body);
  connection_.request(
      method,
      path,
      std::string_view(),  // query
      headers,
      body,
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
          VLOG(1)(
              R"(balance={})"_sv,
              balance);
          handler_(balance);
        } else {
          LOG(WARNING)(
              R"(balance={})"_sv,
              balance);
          LOG(FATAL)("Unexpected"_sv);
        }
      } catch (NetworkError& e) {
        LOG(WARNING)(
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
void Rest::get(std::function<void(const core::Promise<json::Positions> &)> &&callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/0/private/OpenPositions"_sv;
  auto body = random_.create_body();
  auto headers = random_.create_headers(method, path, body);
  connection_.request(
      method,
      path,
      std::string_view(),  // query
      ACCEPT_JSON,
      CONTENT_TYPE_FORM,
      headers,
      body,
      [this, callback{std::move(callback)}](auto &response) {
        profile_.open_positions([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            auto positions = core::json::Parser::create<json::Positions>(response.body(), buffer);
            if (positions.error.empty()) {
              VLOG(1)(R"(positions={})"_sv, positions);
              core::Promise<json::Positions> promise(positions);
              callback(promise);
            } else {
              LOG(WARNING)(R"(positions={})"_sv, positions);
              LOG(FATAL)("Unexpected"_sv);
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
            core::Promise<json::Positions> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

template <>
void Rest::get(std::function<void(const core::Promise<json::Token> &)> &&callback) {
  constexpr auto method = core::http::Method::POST;
  constexpr std::string_view path = "/0/private/GetWebSocketsToken"_sv;
  auto body = random_.create_body();
  auto headers = random_.create_headers(method, path, body);
  connection_.request(
      method,
      path,
      std::string_view(),  // query
      ACCEPT_JSON,
      CONTENT_TYPE_FORM,
      headers,
      body,
      [this, callback{std::move(callback)}](auto &response) {
        profile_.get_web_sockets_token([&]() {
          try {
            response.expect(core::http::Status::OK);
            core::json::Buffer buffer(decode_buffer_);
            json::Result::dispatch<json::Token>(
                response.body(),
                buffer,
                [](const roq::span<std::string_view> &e) {
                  LOG(WARNING)(R"(error=[{}])"_fmt, roq::join(e, ","_sv));
                  LOG(FATAL)("Unexpected"_fmt);
                },
                [&](const json::Token &token) {
                  VLOG(1)(R"(token={})"_sv, token);
                  core::Promise<json::Token> promise(token);
                  callback(promise);
                });
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_sv, typeid(e).name(), e.what());
            core::Promise<json::Token> promise(std::current_exception());
            callback(promise);
          }
        });
      });
}

void Rest::operator()(const core::web::Client::Connected &) {
  handler_(*this);
}

void Rest::operator()(const core::web::Client::Disconnected &) {
  ++counter_.disconnect;
  handler_(*this);
}

void Rest::operator()(const core::web::Client::Latency &latency) {
  server::TraceInfo trace_info;
  ExternalLatency external_latency{
      .name = CONNECTION,
      .latency = latency.sample,
  };
  handler_(external_latency, trace_info);
  latency_.ping.update(latency.sample);
}

}  // namespace kraken
}  // namespace roq

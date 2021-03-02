/* Copyright (c) 2017-2021, Hans Erik Thrane */

#include "roq/kraken/rest.h"

#include <fmt/chrono.h>

#include <utility>

#include "roq/core/json/parser.h"

#include "roq/core/metrics/factory.h"

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
static const auto CONNECTION = "rest"_sv;

static const auto ACCEPT_JSON = "application/json"_sv;
static const auto CONTENT_TYPE_FORM = "application/x-www-form-urlencoded"_sv;

struct create_metrics final : public core::metrics::Factory {
  explicit create_metrics(const std::string_view &function)
      : core::metrics::Factory(Flags::name(), CONNECTION, function) {}
};
}  // namespace

Rest::Rest(Handler &handler, Security &security, core::io::Context &context)
    : handler_(handler), security_(security), connection_(
                                                  *this,
                                                  context,
                                                  core::URI(Flags::rest_uri()),
                                                  ROQ_PACKAGE_NAME,
                                                  true,  // keep alive
                                                  Flags::rest_request_queue_depth(),
                                                  Flags::rest_request_timeout(),
                                                  Flags::rest_rate_limit_interval(),
                                                  Flags::rest_rate_limit_max_requests(),
                                                  Flags::rest_ping_freq(),
                                                  Flags::decode_buffer_size(),
                                                  Flags::encode_buffer_size(),
                                                  Flags::rest_ping_path()),
      decode_buffer_(Flags::decode_buffer_size()),
      counter_{
          .disconnect = create_metrics("disconnect"_sv),
      },
      profile_{
          .assets = create_metrics("assets"_sv),
          .asset_pairs = create_metrics("asset_pairs"_sv),
          .balance = create_metrics("balance"_sv),
          .open_positions = create_metrics("open_positions"_sv),
          .get_web_sockets_token = create_metrics("get_web_sockets_token"_sv),
      },
      latency_{
          .ping = create_metrics("ping"_sv),
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
              VLOG(1)(R"(assets={})"_fmt, assets);
              core::Promise<json::Assets> promise(assets);
              callback(promise);
            } else {
              LOG(WARNING)(R"(assets={})"_fmt, assets);
              LOG(FATAL)("Unexpected"_sv);
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
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
              VLOG(1)(R"(asset_pairs={})"_fmt, asset_pairs);
              core::Promise<json::AssetPairs> promise(asset_pairs);
              callback(promise);
            } else {
              LOG(WARNING)(R"(asset_pairs={})"_fmt, asset_pairs);
              LOG(FATAL)("Unexpected"_sv);
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
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
  auto body = security_.create_body();
  auto headers = security_.create_headers(
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
              R"(balance={})"_fmt,
              balance);
          handler_(balance);
        } else {
          LOG(WARNING)(
              R"(balance={})"_fmt,
              balance);
          LOG(FATAL)("Unexpected"_sv);
        }
      } catch (NetworkError& e) {
        LOG(WARNING)(
            R"(Exception type={}, what="{}")"_fmt,
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
  auto body = security_.create_body();
  auto headers = security_.create_headers(method, path, body);
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
              VLOG(1)(R"(positions={})"_fmt, positions);
              core::Promise<json::Positions> promise(positions);
              callback(promise);
            } else {
              LOG(WARNING)(R"(positions={})"_fmt, positions);
              LOG(FATAL)("Unexpected"_sv);
            }
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
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
  auto body = security_.create_body();
  auto headers = security_.create_headers(method, path, body);
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
                  VLOG(1)(R"(token={})"_fmt, token);
                  core::Promise<json::Token> promise(token);
                  callback(promise);
                });
          } catch (NetworkError &e) {
            LOG(WARNING)
            (R"(Exception type={}, what="{}")"_fmt, typeid(e).name(), e.what());
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
      .stream_id = {},
      .name = CONNECTION,
      .latency = latency.sample,
  };
  handler_(external_latency, trace_info);
  latency_.ping.update(latency.sample);
}

}  // namespace kraken
}  // namespace roq

/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/rest.h"

#include <fmt/format.h>
#include <fmt/chrono.h>

#include "roq/core/json/parser.h"

#include "roq/kraken/gateway.h"
#include "roq/kraken/options.h"

#include "roq/kraken/json/utils.h"

#include "roq/kraken/json/asset_pairs.h"

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
        .asset_pairs = create_profile("asset_pairs"),
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
    .write(_profile.asset_pairs)
    // latency
    .write(_latency.ping);
}

void Rest::get_asset_pairs(
    std::function<void(const core::web::Response&)>&& callback) {
  _connection.request(
      core::http::Method::GET,
      "/public/AssetPairs",
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
                  VLOG(1)(
                      FMT_STRING(R"(asset_pairs={})"),
                      asset_pairs);
                  _gateway(asset_pairs);
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

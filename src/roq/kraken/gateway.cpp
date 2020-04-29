/* Copyright (c) 2017-2020, Hans Erik Thrane */

#include "roq/kraken/gateway.h"

#include <limits>
#include <utility>

#include "roq/core/utils.h"

#include "roq/kraken/options.h"

#include "roq/kraken/json/utils.h"

namespace roq {
namespace kraken {

/*
template <typename T>
static bool mbp_update(
    auto& data,
    size_t& offset,
    const T& item) {
  auto& obj = data[offset];
  new (&obj) MBPUpdate {
    .price = item.price,
    .quantity = item.size,
  };
  ++offset;
  return offset < data.size();
}

template <typename T>
static bool trade_update(
    auto& data,
    size_t& offset,
    const T& item) {
  auto& obj = data[offset];
  new (&obj) Trade {
    .side = json::map(item.side),
    .price = item.price,
    .quantity = item.quantity,
    .trade_id = {},
  };
  // XXX write tradeid
  ++offset;
  return offset < data.size();
}
*/

Gateway::Gateway(
    server::Dispatcher& dispatcher,
    const Config& config)
    : _dispatcher(dispatcher),
      _account(config.get_account()),
      _access_key(config.get_access_key()),
      _random(config.get_access_secret()),
      _dns_base(_base, true),
      _web_socket {
        .connection = {
          *this,
          config,
          _random,
          _base,
          _dns_base,
          _ssl_context,
        },
        .download = WebSocketDownload(
            std::chrono::seconds { FLAGS_download_timeout_secs },
            [this](auto state) {
              return download(state);
            }),
      },
      _rest {
        .connection = {
          *this,
          config,
          _random,
          _base,
          _dns_base,
          _ssl_context,
        },
      },
      _bid(FLAGS_max_depth),
      _ask(FLAGS_max_depth),
      _trade(FLAGS_max_trades) {
  LOG_IF(WARNING, FLAGS_cancel_on_disconnect == false)(
      "Orders will *NOT* be cancelled on disconnect");
}

void Gateway::operator()(const StartEvent& event) {
  LOG(INFO)("Starting the gateway...");
  _web_socket.connection(event);
  _rest.connection(event);
}

void Gateway::operator()(const StopEvent& event) {
  LOG(INFO)("Stopping the gateway...");
  _rest.connection(event);
  _web_socket.connection(event);
}

void Gateway::operator()(const TimerEvent& event) {
  _web_socket.connection(event);
  _rest.connection(event);
  // download
  /*
  if (_web_socket.download.has_expired()) {
    LOG(WARNING)("WebSocket download has timed out");
    _web_socket.download.reset();
    _web_socket.connection.close();
  }
  */
  _base.loop(EVLOOP_NONBLOCK);
}

void Gateway::operator()(const ConnectionStatusEvent&) {
}

void Gateway::operator()(
    const CreateOrderEvent& event,
    const std::string_view& request_id,
    uint32_t gateway_order_id) {
  // TODO
}

void Gateway::operator()(
    const ModifyOrderEvent& event,
    const std::string_view& request_id,
    const server::OMS_Order& order) {
  // TODO
}

void Gateway::operator()(
    const CancelOrderEvent& event,
    const std::string_view& request_id,
    const server::OMS_Order& order) {
  // TODO
}

void Gateway::operator()(Metrics& metrics) {
  _rest.connection(metrics);
  _web_socket.connection(metrics);
}

// rest

void Gateway::operator()(const Rest&) {
}

// web socket

int32_t Gateway::download(WebSocketDownload::State state) {
  if (_web_socket.connection.ready() == false)
    return -1;
  switch (state) {
    case WebSocketDownload::State::UNDEFINED:
      assert(false);
      break;
    case WebSocketDownload::State::DONE:
      update(GatewayStatus::READY);
      return 0;
  }
  assert(false);
  return 0;
}

void Gateway::operator()(const WebSocket&) {
  if (_web_socket.connection.ready()) {
    _web_socket.download.begin();
  } else {
    _web_socket.download.reset();
    _symbols.clear();
  }
}

void Gateway::update(GatewayStatus gateway_status) {
  if (gateway_status == _gateway_status)
    return;
  _gateway_status = gateway_status;
  MarketDataStatus market_data_status {
    .status = _gateway_status,
  };
  enqueue(
      market_data_status,
      false);
  OrderManagerStatus order_manager_status {
    .account = _account,
    .status = _gateway_status,
  };
  enqueue(
      order_manager_status,
      true);
  LOG(INFO)(
      FMT_STRING(R"(Update: gateway_status={})"),
      _gateway_status);
}

template <typename T>
inline void Gateway::enqueue(
    const T& value,
    bool is_last) {
  auto now = core::get_system_clock();
  _dispatcher(
      value,
      now,
      now,
      is_last);
}

template <typename T>
inline void Gateway::enqueue(
    uint8_t user_id,
    const T& value,
    bool is_last) {
  auto now = core::get_system_clock();
  _dispatcher(
      user_id,
      value,
      now,
      now,
      is_last);
}

}  // namespace kraken
}  // namespace roq

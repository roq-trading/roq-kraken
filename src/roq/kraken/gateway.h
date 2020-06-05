/* Copyright (c) 2017-2020, Hans Erik Thrane */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "roq/server.h"
#include "roq/download.h"

#include "roq/core/hash/map.h"
#include "roq/core/hash/set.h"

#include "roq/core/ssl/ssl.h"

#include "roq/core/event/base.h"
#include "roq/core/event/dns_base.h"

#include "roq/kraken/config.h"
#include "roq/kraken/random.h"
#include "roq/kraken/rest.h"
#include "roq/kraken/web_socket_private.h"
#include "roq/kraken/web_socket_public.h"

#include "roq/kraken/private_state.h"
#include "roq/kraken/public_state.h"

#include "roq/kraken/json/asset_pairs.h"
#include "roq/kraken/json/assets.h"
#include "roq/kraken/json/positions.h"
#include "roq/kraken/json/token.h"

namespace roq {
namespace kraken {

class Gateway final
    : public server::Handler,
      public Rest::Handler,
      public WebSocketPublic::Handler,
      public WebSocketPrivate::Handler {
 public:
  Gateway(
      server::Dispatcher& dispatcher,
      const Config& config);

 protected:
  // server::Handler

  void operator()(const server::StartEvent&) override;
  void operator()(const server::StopEvent&) override;
  void operator()(const server::TimerEvent&) override;
  void operator()(const server::ConnectionStatusEvent&) override;

  void operator()(
      const CreateOrderEvent& event,
      const std::string_view& request_id,
      uint32_t gateway_order_id) override;
  void operator()(
      const ModifyOrderEvent& event,
      const std::string_view& request_id,
      const server::OMS_Order& order) override;
  void operator()(
      const CancelOrderEvent& event,
      const std::string_view& request_id,
      const server::OMS_Order& order) override;

  void operator()(Metrics& metrics) override;

  // Rest::Handler

  void operator()(const Rest&) override;

  // WebSocketPublic::Handler

  void operator()(const WebSocketPublic&) override;

  void operator()(
      const json::Trade& trade,
      const std::string_view& pair) override;
  void operator()(
      const json::Spread& spread,
      const std::string_view& pair) override;
  void operator()(
      const json::Book& book,
      const std::string_view& pair) override;

  // WebSocketPrivate::Handler

  void operator()(const WebSocketPrivate&) override;

  void operator()(const json::AddOrderStatus&) override;
  void operator()(const json::CancelOrderStatus&) override;

  void operator()(const json::OpenOrders&) override;
  void operator()(const json::OwnTrades&) override;

 private:
  void operator()(const json::Assets&);
  void operator()(const json::AssetPairs&);
  void operator()(const json::Positions&);
  void operator()(const json::Token&);

  using WebSocketDownload = server::Download<PublicState>;

  int32_t download(WebSocketDownload::State state);

  using WebSocketPrivateDownload = server::Download<PrivateState>;

  int32_t download(WebSocketPrivateDownload::State state);

  void update(GatewayStatus gateway_status);

  // public

  void download_assets();
  void download_asset_pairs();

  void download_balance();
  void download_open_positions();

  void subscribe_public();

  // private

  void download_web_sockets_token();

  void subscribe_private();

  template <typename T>
  void enqueue(
      const T& value,
      bool is_last);

  template <typename T>
  void enqueue(
      uint8_t user_id,
      const T& value,
      bool is_last);

 private:
  server::Dispatcher& _dispatcher;
  // config
  const std::string _account;
  const std::string _access_key;
  // authentication
  Random _random;
  // async
  core::event::Base _base;
  core::event::DNSBase _dns_base;
  // crypto
  core::ssl::Context _ssl_context;
  // connections
  struct {
    WebSocketPublic connection;
    WebSocketDownload download;
  } _web_socket_public;
  struct {
    WebSocketPrivate connection;
    WebSocketPrivateDownload download;
  } _web_socket_private;
  struct {
    Rest connection;
  } _rest;
  // download (web socket)
  std::vector<std::string> _symbols;
  // market data + order manager
  GatewayStatus _gateway_status = GatewayStatus::DISCONNECTED;
  // market data
  core::page_aligned_vector<MBPUpdate> _bid, _ask;
  core::page_aligned_vector<Trade> _trade;

  // experimental
  std::string _token;
};

}  // namespace kraken
}  // namespace roq

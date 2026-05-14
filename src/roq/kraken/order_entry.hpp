/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/core/download.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/kraken/account.hpp"
#include "roq/kraken/shared.hpp"

#include "roq/kraken/json/balance_ack.hpp"
#include "roq/kraken/json/open_orders_ack.hpp"
#include "roq/kraken/json/open_positions_ack.hpp"
#include "roq/kraken/json/token_ack.hpp"
#include "roq/kraken/json/trade_balance_ack.hpp"

namespace roq {
namespace kraken {

struct OrderEntry final : public web::rest::Client::Handler {
  struct TokenUpdate final {
    std::string_view account;
    std::string_view token;
  };

  struct Handler {
    virtual void operator()(Trace<StreamStatus> const &) = 0;
    virtual void operator()(Trace<ExternalLatency> const &) = 0;
    // cross-communication
    virtual void operator()(TokenUpdate &) = 0;
    virtual void operator()(Trace<FundsUpdate> const &, bool is_last) = 0;
  };

  OrderEntry(Handler &, io::Context &context, uint16_t stream_id, Account &, Shared &);

  OrderEntry(OrderEntry const &) = delete;

  bool ready() const { return connection_status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &) const;

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id);
  uint16_t operator()(
      Event<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);
  uint16_t operator()(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id);

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id);

 protected:
  // web::rest::Client::Handler

  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;

  void operator()(ConnectionStatus, std::string_view const &reason = {});

  enum class State {
    UNDEFINED = 0,
    TOKEN,
    BALANCE,
    TRADE_BALANCE,
    OPEN_POSITIONS,
    OPEN_ORDERS,
    DONE,
  };

  uint32_t download(State);

  void get_token();
  void get_token_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::TokenAck> const &);

  void get_balance();
  void get_balance_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::BalanceAck> const &);

  void get_trade_balance();
  void get_trade_balance_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::TradeBalanceAck> const &);

  void get_open_positions();
  void get_open_positions_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::OpenPositionsAck> const &);

  void get_open_orders();
  void get_open_orders_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::OpenOrdersAck> const &);

  // helpers

  template <typename SuccessHandler, typename ErrorHandler>
  void process_response(web::rest::Response const &, SuccessHandler, ErrorHandler);

  template <typename... Args>
  void operator()(Trace<server::oms::Response> const &, uint8_t user_id, uint64_t order_id, Args &&...);

  void operator()(Trace<server::oms::OrderUpdate> const &, std::string_view const &client_order_id);

 private:
  Handler &handler_;
  // config
  uint16_t const stream_id_;
  std::string const name_;
  // connection
  std::unique_ptr<web::rest::Client> const connection_;
  // buffers
  core::json::BufferStack decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile get_web_sockets_token, get_web_sockets_token_ack, balance, balance_ack, trade_balance, trade_balance_ack, open_positions,
        open_positions_ack, open_orders, open_orders_ack;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus connection_status_ = {};
  core::Download<State> download_;
};

}  // namespace kraken
}  // namespace roq

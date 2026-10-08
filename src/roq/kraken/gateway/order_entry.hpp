/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/utils/metrics/counter.hpp"
#include "roq/utils/metrics/latency.hpp"
#include "roq/utils/metrics/profile.hpp"

#include "roq/io/context.hpp"

#include "roq/web/rest/client.hpp"

#include "roq/core/download_2.hpp"

#include "roq/core/json/buffer_stack.hpp"

#include "roq/server.hpp"

#include "roq/server/stream.hpp"

#include "roq/kraken/gateway/account.hpp"
#include "roq/kraken/gateway/shared.hpp"

#include "roq/kraken/protocol/json/balance_ack.hpp"
#include "roq/kraken/protocol/json/open_orders_ack.hpp"
#include "roq/kraken/protocol/json/open_positions_ack.hpp"
#include "roq/kraken/protocol/json/token_ack.hpp"
#include "roq/kraken/protocol/json/trade_balance_ack.hpp"

namespace roq {
namespace kraken {
namespace gateway {

struct OrderEntry final : public Base<OrderEntry>, public server::OrderActionStream, public web::rest::Client::Handler {
  struct TokenUpdate final {
    std::string_view account;
    std::string_view token;
  };

  struct Handler {
    virtual void operator()(TokenUpdate &) = 0;
  };

  OrderEntry(Handler &, io::Context &context, uint16_t stream_id, Account &, Shared &);

  // protected:
  friend base_type;

  // server::Stream

  uint16_t stream_id() const override { return stream_id_; }

  bool ready() const override { return connection_status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;

  void operator()(metrics::Writer &) const override;

  void operator()(Trace<ConnectionStatus> const &, std::string_view const &reason = {}) override;

  // server::OrderActionStream

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

 protected:
  // web::rest::Client::Handler

  void operator()(Trace<web::rest::Connected> const &) override;
  void operator()(Trace<web::rest::Disconnected> const &) override;
  void operator()(Trace<web::rest::Latency> const &) override;

  // core::Download

  enum class State {
    UNDEFINED = 0,
    TOKEN,
    BALANCE,
    TRADE_BALANCE,
    OPEN_POSITIONS,
    OPEN_ORDERS,
    DONE,
  };

  int32_t download(Trace<State> const &);

  // token

  void get_token();
  void get_token_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::TokenAck> const &);

  // balance

  void get_balance();
  void get_balance_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::BalanceAck> const &);

  // trade-balance

  void get_trade_balance();
  void get_trade_balance_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::TradeBalanceAck> const &);

  // open-positions

  void get_open_positions();
  void get_open_positions_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::OpenPositionsAck> const &);

  // open-orders

  void get_open_orders();
  void get_open_orders_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<protocol::json::OpenOrdersAck> const &);

  // helpers

  void process_response(Trace<web::rest::Response> const &, auto error_handler, auto success_handler);

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
  core::Download2<State> download_;
};

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

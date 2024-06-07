/* Copyright (c) 2017-2024, Hans Erik Thrane */

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

#include "roq/server.hpp"

#include "roq/kraken/account.hpp"
#include "roq/kraken/order_entry_state.hpp"
#include "roq/kraken/shared.hpp"

#include "roq/kraken/json/positions.hpp"
#include "roq/kraken/json/token.hpp"

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
  };

  OrderEntry(Handler &, io::Context &context, uint16_t stream_id, Account &, Shared &);

  OrderEntry(OrderEntry &&) = delete;
  OrderEntry(OrderEntry const &) = delete;

  bool ready() const { return status_ == ConnectionStatus::READY; }

  void operator()(Event<Start> const &);
  void operator()(Event<Stop> const &);
  void operator()(Event<Timer> const &);

  void operator()(metrics::Writer &);

  uint16_t operator()(Event<CreateOrder> const &, server::oms::Order const &, std::string_view const &request_id);
  uint16_t operator()(Event<ModifyOrder> const &, server::oms::Order const &, std::string_view const &request_id, std::string_view const &previous_request_id);
  uint16_t operator()(Event<CancelOrder> const &, server::oms::Order const &, std::string_view const &request_id, std::string_view const &previous_request_id);

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id);

 protected:
  void operator()(Trace<web::rest::Client::Connected> const &) override;
  void operator()(Trace<web::rest::Client::Disconnected> const &) override;
  void operator()(Trace<web::rest::Client::Latency> const &) override;

  void operator()(ConnectionStatus);

  uint32_t download(OrderEntryState);

  void get_token();
  void get_token_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::Token> const &);

  void get_positions();
  void get_positions_ack(Trace<web::rest::Response> const &, uint32_t sequence);
  void operator()(Trace<json::Positions> const &);

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
  std::vector<std::byte> decode_buffer_;
  // metrics
  struct {
    utils::metrics::Counter disconnect;
  } counter_;
  struct {
    utils::metrics::Profile get_web_sockets_token, get_web_sockets_token_ack, positions, positions_ack;
  } profile_;
  struct {
    utils::metrics::Latency ping;
  } latency_;
  // account
  Account &account_;
  // cache
  Shared &shared_;
  // state
  ConnectionStatus status_ = {};
  core::Download<OrderEntryState> download_;
};

}  // namespace kraken
}  // namespace roq

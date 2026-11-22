/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <string>
#include <vector>

#include "roq/server.hpp"

#include "roq/io/context.hpp"

#include "roq/kraken/gateway/account.hpp"
#include "roq/kraken/gateway/config.hpp"
#include "roq/kraken/gateway/settings.hpp"
#include "roq/kraken/gateway/shared.hpp"

#include "roq/kraken/gateway/drop_copy.hpp"
#include "roq/kraken/gateway/market_data.hpp"
#include "roq/kraken/gateway/order_entry.hpp"

namespace roq {
namespace kraken {
namespace gateway {

struct Controller final : public Base<Controller>, public server::Handler, public OrderEntry::Handler, public MarketData::Handler, public DropCopy::Handler {
  ROQ_PUBLIC static std::unique_ptr<server::Handler> create(server::Dispatcher &, Settings const &, Config const &, io::Context &);

  ROQ_PUBLIC static uint8_t parse_api(Settings const &);

  Controller(server::Dispatcher &, Settings const &, Config const &, io::Context &);

  Controller(Controller const &) = delete;

 protected:
  friend base_type;

  // server::Handler

  void operator()(Trace<Start> const &) override;
  void operator()(Trace<Stop> const &) override;
  void operator()(Trace<Timer> const &) override;

  void operator()(Trace<Connected> const &, uint8_t user_id) override;
  void operator()(Trace<Disconnected> const &, uint8_t user_id) override;

  void operator()(Trace<Subscribe> const &) override;
  void operator()(Trace<Control> const &) override;

  void operator()(Trace<server::oms::OrderActionBatchBegin> const &) override;
  void operator()(Trace<server::oms::OrderActionBatchCommit> const &) override;
  void operator()(Trace<server::oms::OrderActionBatchRollback> const &) override;

  uint16_t operator()(Trace<CreateOrder> const &, server::oms::Order const &, server::oms::RefData const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Trace<ModifyOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Trace<CancelOrder> const &,
      server::oms::Order const &,
      server::oms::RefData const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Trace<CancelAllOrders> const &, std::string_view const &request_id) override;

  uint16_t operator()(Trace<MassQuote> const &) override;

  uint16_t operator()(Trace<CancelQuotes> const &) override;

  void operator()(metrics::Writer &) const override;

  // MarketData::Handler

  void operator()(Trace<SymbolsUpdate> const &) override;

  // OrderEntry::Handler

  void operator()(Trace<TokenUpdate> const &) override;

  // helpers

  void ensure_symbol_slices(TraceInfo const &, size_t size);

  template <typename... Args>
  void dispatch(Args &&...);

  template <typename... Args>
  static void dispatch_helper(auto &self, Args &&...);

  OrderEntry &get_order_entry(std::string_view const &account);
  DropCopy &get_drop_copy(std::string_view const &account);

  template <typename Res, typename T, typename... Args>
  Res dispatch_order_action(Trace<T> const &, Args &&...);

 private:
  server::Dispatcher &dispatcher_;
  //
  io::Context &context_;
  Shared shared_;
  uint16_t stream_id_ = {};
  //
  utils::unordered_map<std::string, std::unique_ptr<Account>> const accounts_;
  //
  std::vector<std::unique_ptr<MarketData>> market_data_;
  utils::unordered_map<std::string, std::unique_ptr<OrderEntry>> order_entry_;
  utils::unordered_map<std::string, std::unique_ptr<DropCopy>> drop_copy_;
};

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

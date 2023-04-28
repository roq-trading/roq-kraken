/* Copyright (c) 2017-2023, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <vector>

#include "roq/server.hpp"

#include "roq/io/context.hpp"

#include "roq/kraken/account.hpp"
#include "roq/kraken/config.hpp"
#include "roq/kraken/drop_copy.hpp"
#include "roq/kraken/market_data.hpp"
#include "roq/kraken/order_entry.hpp"
#include "roq/kraken/rest.hpp"
#include "roq/kraken/shared.hpp"

namespace roq {
namespace kraken {

struct Gateway final : public server::Handler,
                       public Rest::Handler,
                       public OrderEntry::Handler,
                       public MarketData::Handler,
                       public DropCopy::Handler {
  Gateway(server::Dispatcher &, Config const &, io::Context &);

 protected:
  // server::Handler

  void operator()(Event<Start> const &) override;
  void operator()(Event<Stop> const &) override;
  void operator()(Event<Timer> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  uint16_t operator()(Event<CreateOrder> const &, oms::Order const &, std::string_view const &request_id) override;
  uint16_t operator()(
      Event<ModifyOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;
  uint16_t operator()(
      Event<CancelOrder> const &,
      oms::Order const &,
      std::string_view const &request_id,
      std::string_view const &previous_request_id) override;

  uint16_t operator()(Event<CancelAllOrders> const &, std::string_view const &request_id) override;

  void operator()(metrics::Writer &) override;

  void operator()(Trace<StreamStatus> const &) override;
  void operator()(Trace<ExternalLatency> const &) override;
  void operator()(Trace<ReferenceData> const &, bool is_last) override;
  void operator()(Trace<MarketStatus> const &, bool is_last) override;
  void operator()(Trace<TopOfBook> const &, bool is_last) override;
  void operator()(Trace<MarketByPriceUpdate> const &, bool is_last) override;
  void operator()(Trace<TradeSummary> const &, bool is_last) override;

  void operator()(OrderEntry::TokenUpdate &) override;
  void operator()(Rest::SymbolsUpdate &) override;

  void ensure_symbol_slices(size_t size);

  // utilities

  template <typename... Args>
  void dispatch(Args &&...);

  OrderEntry &get_order_entry(std::string_view const &account);

 private:
  server::Dispatcher &dispatcher_;
  // accounts
  absl::flat_hash_map<std::string, std::unique_ptr<Account>> const accounts_;
  // io
  io::Context &context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  Rest rest_;
  absl::flat_hash_map<std::string, std::unique_ptr<OrderEntry>> order_entry_;
  absl::flat_hash_map<std::string, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
  // cache
  std::vector<MBPUpdate> bids_, asks_;
};

}  // namespace kraken
}  // namespace roq

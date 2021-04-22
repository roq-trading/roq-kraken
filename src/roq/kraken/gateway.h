/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <vector>

#include "roq/server.h"

#include "roq/core/io/context.h"

#include "roq/kraken/config.h"
#include "roq/kraken/drop_copy.h"
#include "roq/kraken/market_data.h"
#include "roq/kraken/order_entry.h"
#include "roq/kraken/security.h"
#include "roq/kraken/shared.h"

namespace roq {
namespace kraken {

class Gateway final : public server::Handler,
                      public OrderEntry::Handler,
                      public MarketData::Handler,
                      public DropCopy::Handler {
 public:
  Gateway(server::Dispatcher &, const Config &);

 protected:
  // server::Handler

  void operator()(const Event<Start> &) override;
  void operator()(const Event<Stop> &) override;
  void operator()(const Event<Timer> &) override;
  void operator()(const Event<Connected> &) override;
  void operator()(const Event<Disconnected> &) override;

  void operator()(
      const Event<CreateOrder> &,
      const std::string_view &request_id,
      uint32_t gateway_order_id) override;
  void operator()(
      const Event<ModifyOrder> &,
      const std::string_view &request_id,
      const server::OMS_Order &) override;
  void operator()(
      const Event<CancelOrder> &,
      const std::string_view &request_id,
      const server::OMS_Order &) override;
  void operator()(const Event<CancelAllOrders> &, const std::string_view &request_id) override;

  void operator()(metrics::Writer &) override;

  void operator()(const server::Trace<StreamStatus> &) override;
  void operator()(const server::Trace<ExternalLatency> &) override;
  void operator()(const server::Trace<ReferenceData> &, bool is_last) override;
  void operator()(const server::Trace<MarketStatus> &, bool is_last) override;
  void operator()(const server::Trace<TopOfBook> &, bool is_last) override;
  void operator()(const server::Trace<MarketByPriceUpdate> &, bool is_last) override;
  void operator()(const server::Trace<TradeSummary> &, bool is_last) override;

  void operator()(OrderEntry::TokenUpdate &) override;
  void operator()(OrderEntry::SymbolsUpdate &) override;

  // utilities

  OrderEntry &get_order_entry(const std::string_view &account);

 private:
  server::Dispatcher &dispatcher_;
  // config
  const std::string master_account_;
  // security
  absl::flat_hash_map<std::string, std::unique_ptr<Security>> security_;
  // io
  core::io::Context context_;
  // shared
  Shared shared_;
  // seed
  uint16_t stream_id_ = {};
  // streams
  absl::flat_hash_map<std::string, std::unique_ptr<OrderEntry>> order_entry_;
  absl::flat_hash_map<std::string, std::unique_ptr<DropCopy>> drop_copy_;
  std::vector<std::unique_ptr<MarketData>> market_data_;
};

}  // namespace kraken
}  // namespace roq

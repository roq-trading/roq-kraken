/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/kraken/gateway/controller.hpp"

#include <utility>

#include "roq/server/oms/exceptions.hpp"

using namespace std::literals;

namespace roq {
namespace kraken {
namespace gateway {

// === HELPERS ===

namespace {
template <typename R>
R create_accounts(auto &config) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[_, iter] : config.accounts) {
    result.try_emplace(static_cast<std::string_view>(iter.name), std::make_unique<Account>(config, iter.name));
  }
  return result;
}

template <typename R>
R create_market_data(auto &gateway, auto &context, auto &stream_id, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  auto market_data = std::make_unique<MarketData>(gateway, context, ++stream_id, shared, 0);
  result.emplace_back(std::move(market_data));
  return result;
}

template <typename R>
R create_order_entry(auto &gateway, auto &context, auto &stream_id, auto &accounts, auto &shared) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[name, account] : accounts) {
    result.try_emplace(static_cast<std::string_view>(name), std::make_unique<OrderEntry>(gateway, context, ++stream_id, *account, shared));
  }
  return result;
}

template <typename R>
R create_drop_copy(auto &accounts) {
  using result_type = std::remove_cvref_t<R>;
  result_type result;
  for (auto &[name, account] : accounts) {
    result.try_emplace(static_cast<std::string_view>(name), nullptr);
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<server::Handler> Controller::create(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context) {
  return std::make_unique<Controller>(dispatcher, settings, config, context);
}

uint8_t Controller::parse_api(Settings const &) {
  return {};
}

Controller::Controller(server::Dispatcher &dispatcher, Settings const &settings, Config const &config, io::Context &context)
    : dispatcher_{dispatcher}, accounts_{create_accounts<decltype(accounts_)>(config)}, context_{context}, shared_{dispatcher, settings},
      market_data_{create_market_data<decltype(market_data_)>(*this, context_, stream_id_, shared_)},
      order_entry_{create_order_entry<decltype(order_entry_)>(*this, context_, stream_id_, accounts_, shared_)},
      drop_copy_{create_drop_copy<decltype(drop_copy_)>(accounts_)} {
}

// server::Handler

void Controller::operator()(Event<Start> const &event) {
  log::info("Starting..."sv);
  dispatch(event);
}

void Controller::operator()(Event<Stop> const &event) {
  log::info("Stopping..."sv);
  dispatch(event);
}

void Controller::operator()(Event<Timer> const &event) {
  dispatch(event);
}

void Controller::operator()(Event<Control> const &event) {
  auto &[message_info, control] = event;
  switch (control.action) {
    using enum Action;
    case UNDEFINED:
      assert(false);
      break;
    case ENABLE:
      dispatcher_(State::ENABLED);
      break;
    case DISABLE:
      dispatcher_(State::DISABLED);
      break;
  }
}

void Controller::operator()(Event<Connected> const &) {
}

void Controller::operator()(Event<Disconnected> const &) {
}

void Controller::operator()(Event<Subscribe> const &event) {
  auto &[message_info, subscribe] = event;
  std::vector<Symbol> symbols;
  for (auto &item : subscribe.symbols) {
    if (shared_.all_symbols.emplace(item).second) {
      symbols.emplace_back(item);
    } else {
      log::warn(R"(*** DUPLICATE SUBSCRIPTION *** (symbol="{}")"sv, item);
    }
  }
  auto symbols_update = MarketData::SymbolsUpdate{
      .symbols = symbols,
  };
  (*this)(symbols_update);
}

uint16_t Controller::operator()(
    Event<CreateOrder> const &event, server::oms::Order const &order, server::oms::RefData const &ref_data, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  if (shared_.settings.ws_api) {
    return get_drop_copy(event.value.account)(event, order, ref_data, request_id);
  } else {
    return get_order_entry(event.value.account)(event, order, ref_data, request_id);
  }
}

uint16_t Controller::operator()(
    Event<ModifyOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  if (shared_.settings.ws_api) {
    return get_drop_copy(event.value.account)(event, order, ref_data, request_id, previous_request_id);
  } else {
    return get_order_entry(event.value.account)(event, order, ref_data, request_id, previous_request_id);
  }
}

uint16_t Controller::operator()(
    Event<CancelOrder> const &event,
    server::oms::Order const &order,
    server::oms::RefData const &ref_data,
    std::string_view const &request_id,
    std::string_view const &previous_request_id) {
  assert(!std::empty(event.value.account));
  assert(event.value.account == order.account);
  if (shared_.settings.ws_api) {
    return get_drop_copy(event.value.account)(event, order, ref_data, request_id, previous_request_id);
  } else {
    return get_order_entry(event.value.account)(event, order, ref_data, request_id, previous_request_id);
  }
}

uint16_t Controller::operator()(Event<CancelAllOrders> const &event, std::string_view const &request_id) {
  assert(!std::empty(event.value.account));
  if (shared_.settings.ws_api) {
    return get_drop_copy(event.value.account)(event, request_id);
  } else {
    return get_order_entry(event.value.account)(event, request_id);
  }
}

uint16_t Controller::operator()(Event<MassQuote> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

uint16_t Controller::operator()(Event<CancelQuotes> const &) {
  throw server::oms::NotSupported{"not supported"sv};
}

void Controller::operator()(metrics::Writer &writer) const {
  dispatch_helper(*this, writer);
}

// streams

void Controller::operator()(Trace<StreamStatus> const &event) {
  dispatcher_(event);
}

void Controller::operator()(Trace<ExternalLatency> const &event) {
  dispatcher_(event);
}

void Controller::operator()(Trace<ReferenceData> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Controller::operator()(Trace<MarketStatus> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Controller::operator()(Trace<TopOfBook> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Controller::operator()(Trace<MarketByPriceUpdate> const &event, bool is_last) {
  auto callback = []([[maybe_unused]] auto &market_by_price) {};
  dispatcher_(event, is_last, bids_, asks_, callback);
}

void Controller::operator()(Trace<TradeSummary> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Controller::operator()(Trace<StatisticsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Controller::operator()(Trace<TradeUpdate> const &event, bool is_last, uint8_t user_id, std::string_view const &request_id) {
  dispatcher_(event, is_last, user_id, request_id);
}

void Controller::operator()(Trace<FundsUpdate> const &event, bool is_last) {
  dispatcher_(event, is_last);
}

void Controller::operator()(MarketData::SymbolsUpdate &symbols_update) {
  auto [size, start_from] = shared_.symbols(symbols_update.symbols);
  ensure_symbol_slices(size);
  for (auto &iter : market_data_) {
    (*iter).subscribe(start_from);
  }
}

void Controller::operator()(OrderEntry::TokenUpdate &token_update) {
  auto &account = token_update.account;
  assert(!std::empty(account));
  auto iter = drop_copy_.find(account);
  if (iter == std::end(drop_copy_)) [[unlikely]] {
    log::fatal(R"(Unexpected: account="{}")"sv, account);
  }
  if (!static_cast<bool>((*iter).second)) {
    log::info("Create drop-copy (ws-private)"sv);
    auto drop_copy = std::make_unique<DropCopy>(*this, context_, ++stream_id_, *accounts_.at(account), shared_, token_update.token);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*drop_copy, message_info, start);
    (*iter).second = std::move(drop_copy);
  }
}

// utilities

void Controller::ensure_symbol_slices(size_t size) {
  while (std::size(market_data_) < size) {
    auto stream_id = ++stream_id_;
    auto index = std::size(market_data_);
    log::debug("Create MarketData (stream_id={}, index={})"sv, stream_id, index);
    auto market_data = std::make_unique<MarketData>(*this, context_, stream_id, shared_, index);
    MessageInfo message_info;
    Start start;
    create_event_and_dispatch(*market_data, message_info, start);
    market_data_.emplace_back(std::move(market_data));
  }
}

template <typename... Args>
void Controller::dispatch(Args &&...args) {
  dispatch_helper(*this, std::forward<Args>(args)...);
}

template <typename... Args>
void Controller::dispatch_helper(auto &self, Args &&...args) {
  auto helper = [&](auto &target) { target(std::forward<Args>(args)...); };
  for (auto &item : self.market_data_) {
    helper(*item);
  }
  for (auto &[_, item] : self.order_entry_) {
    helper(*item);
  }
  for (auto &[_, item] : self.drop_copy_) {
    if (static_cast<bool>(item)) {
      helper(*item);
    }
  }
}

OrderEntry &Controller::get_order_entry(std::string_view const &account) {
  auto iter = order_entry_.find(account);
  if (iter != std::end(order_entry_)) {
    return *(*iter).second;
  }
  throw RuntimeError{R"(Unknown account="{}")"sv, account};
}

DropCopy &Controller::get_drop_copy(std::string_view const &account) {
  auto iter = drop_copy_.find(account);
  if (iter != std::end(drop_copy_)) {
    return *(*iter).second;
  }
  throw RuntimeError{R"(Unknown account="{}")"sv, account};
}

}  // namespace gateway
}  // namespace kraken
}  // namespace roq

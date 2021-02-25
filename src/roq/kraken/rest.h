/* Copyright (c) 2017-2021, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/core/promise.h"

#include "roq/core/utils/buffer.h"

#include "roq/core/metrics/counter.h"
#include "roq/core/metrics/latency.h"
#include "roq/core/metrics/profile.h"

#include "roq/core/io/context.h"

#include "roq/core/web/client.h"

#include "roq/server.h"

#include "roq/kraken/security.h"

namespace roq {
namespace kraken {

class Rest final : public core::web::Client::Handler {
 public:
  struct Handler {
    virtual void operator()(const Rest &) = 0;
    virtual void operator()(const ExternalLatency &, const server::TraceInfo &) = 0;
  };

  Rest(Handler &handler, Security &security, core::io::Context &context);

  Rest(Rest &&) = delete;
  Rest(const Rest &) = delete;

  bool ready() const;

  void operator()(const Event<Start> &);
  void operator()(const Event<Stop> &);
  void operator()(const Event<Timer> &);

  void operator()(metrics::Writer &writer);

  template <typename T>
  void get(std::function<void(const core::Promise<T> &)> &&callback);

 protected:
  void operator()(const core::web::Client::Connected &) override;
  void operator()(const core::web::Client::Disconnected &) override;
  void operator()(const core::web::Client::Latency &) override;

 private:
  Handler &handler_;
  // authentication
  Security &security_;
  // connection
  core::web::Client connection_;
  // buffers
  core::utils::Buffer decode_buffer_;
  // metrics
  struct {
    core::metrics::Counter disconnect;
  } counter_;
  struct {
    core::metrics::Profile assets, asset_pairs, balance, open_positions, get_web_sockets_token;
  } profile_;
  struct {
    core::metrics::Latency ping;
  } latency_;
};

}  // namespace kraken
}  // namespace roq

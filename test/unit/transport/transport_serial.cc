/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <memory>

#include "unilink/config/serial_config.hpp"
#include "unilink/interface/iserial_port.hpp"
#include "unilink/transport/serial/serial.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace std::chrono_literals;

namespace {

// Minimal fake serial port to avoid real device access in tests
class FakeSerialPort : public interface::SerialPortInterface {
 public:
  explicit FakeSerialPort(boost::asio::io_context& ioc) : ioc_(ioc) {}

  void open(const std::string&, boost::system::error_code& ec) override {
    open_ = true;
    ec.clear();
  }
  bool is_open() const override { return open_; }
  void close(boost::system::error_code& ec) override {
    open_ = false;
    ec.clear();
  }

  void set_option(const boost::asio::serial_port_base::baud_rate&, boost::system::error_code& ec) override {
    ec.clear();
  }
  void set_option(const boost::asio::serial_port_base::character_size&, boost::system::error_code& ec) override {
    ec.clear();
  }
  void set_option(const boost::asio::serial_port_base::stop_bits&, boost::system::error_code& ec) override {
    ec.clear();
  }
  void set_option(const boost::asio::serial_port_base::parity&, boost::system::error_code& ec) override { ec.clear(); }
  void set_option(const boost::asio::serial_port_base::flow_control&, boost::system::error_code& ec) override {
    ec.clear();
  }

  void async_read_some(const boost::asio::mutable_buffer&,
                       std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    read_handler_ = std::move(handler);
  }

  void async_write(const boost::asio::const_buffer& buffer,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    boost::system::error_code ok;
    auto size = buffer.size();
    boost::asio::post(ioc_, [handler = std::move(handler), ok, size]() { handler(ok, size); });
  }

  void emit_read(std::size_t n = 1, const boost::system::error_code& ec = {}) {
    if (!read_handler_) return;
    auto handler = std::move(read_handler_);
    boost::asio::post(ioc_, [handler = std::move(handler), ec, n]() { handler(ec, n); });
  }

  void emit_operation_aborted() {
    if (!read_handler_) return;
    auto handler = std::move(read_handler_);
    boost::asio::post(ioc_, [handler]() { handler(make_error_code(boost::asio::error::operation_aborted), 0); });
  }

 private:
  boost::asio::io_context& ioc_;
  bool open_{false};
  std::function<void(const boost::system::error_code&, std::size_t)> read_handler_;
};

}  // namespace

// Ensure destructor path is safe when never started
TEST(TransportSerialTest, DestructorWithoutStartIsSafe) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  auto port = std::make_unique<FakeSerialPort>(ioc);
  EXPECT_NO_THROW({ auto serial = Serial::create(cfg, std::move(port), ioc); });
}

TEST(TransportSerialTest, CreateProvidesSharedFromThis) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);
  EXPECT_NO_THROW({
    auto self = serial->shared_from_this();
    EXPECT_EQ(self.get(), serial.get());
  });
  serial->stop();
}

// operation_aborted after stop must not trigger reconnect/reopen
TEST(TransportSerialTest, StopPreventsReopenAfterOperationAborted) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.retry_interval_ms = 20;
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto* port_raw = port.get();

  auto serial = Serial::create(cfg, std::move(port), ioc);

  std::atomic<bool> stop_called{false};
  std::atomic<int> reconnect_after_stop{0};
  serial->on_state([&](common::LinkState state) {
    if (stop_called.load() && state == common::LinkState::Connecting) {
      reconnect_after_stop.fetch_add(1);
    }
  });

  serial->start();
  ioc.run_for(5ms);  // allow open/configure and first read to post

  stop_called.store(true);
  serial->stop();

  // Simulate read completion with operation_aborted after stop
  port_raw->emit_operation_aborted();

  ioc.run_for(50ms);
  EXPECT_EQ(reconnect_after_stop.load(), 0);
}

TEST(TransportSerialTest, QueueLimitMovesSerialToError) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.backpressure_threshold = 1024;

  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  std::atomic<bool> error_seen{false};
  serial->on_state([&](common::LinkState state) {
    if (state == common::LinkState::Error) error_seen = true;
  });

  serial->start();

  // 2MB buffer exceeds default limit (usually 1MB)
  std::vector<uint8_t> huge(2 * 1024 * 1024, 0xEF);
  serial->async_write_copy(huge.data(), huge.size());

  ioc.run_for(50ms);

  EXPECT_TRUE(error_seen.load());
  serial->stop();
}

TEST(TransportSerialTest, MoveWriteRespectsQueueLimit) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.backpressure_threshold = 1024;

  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  std::atomic<bool> error_seen{false};
  serial->on_state([&](common::LinkState state) {
    if (state == common::LinkState::Error) error_seen = true;
  });

  serial->start();

  std::vector<uint8_t> huge(2 * 1024 * 1024, 0xCD);
  serial->async_write_move(std::move(huge));

  ioc.run_for(50ms);

  EXPECT_TRUE(error_seen.load());
  serial->stop();
}

TEST(TransportSerialTest, SharedWriteRespectsQueueLimit) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.backpressure_threshold = 1024;

  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  std::atomic<bool> error_seen{false};
  serial->on_state([&](common::LinkState state) {
    if (state == common::LinkState::Error) error_seen = true;
  });

  serial->start();

  auto huge = std::make_shared<const std::vector<uint8_t>>(2 * 1024 * 1024, 0xAB);
  serial->async_write_shared(huge);

  ioc.run_for(50ms);

  EXPECT_TRUE(error_seen.load());
  serial->stop();
}

TEST(TransportSerialTest, CallbackExceptionStopsWhenConfigured) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.stop_on_callback_exception = true;
  cfg.retry_interval_ms = 10;

  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto* port_raw = port.get();
  auto serial = Serial::create(cfg, std::move(port), ioc);

  std::atomic<bool> error_seen{false};
  serial->on_state([&](common::LinkState state) {
    if (state == common::LinkState::Error) error_seen = true;
  });

  serial->on_bytes([](const uint8_t*, size_t) { throw std::runtime_error("boom"); });

  serial->start();
  ioc.run_for(5ms);  // allow start to set up read handler

  // Simulate a successful read that triggers the throwing callback.
  port_raw->emit_read(4);

  ioc.run_for(20ms);

  EXPECT_TRUE(error_seen.load());
  serial->stop();
}

TEST(TransportSerialTest, CallbackExceptionRetriesWhenAllowed) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.stop_on_callback_exception = false;
  cfg.retry_interval_ms = 10;

  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto* port_raw = port.get();
  auto serial = Serial::create(cfg, std::move(port), ioc);

  std::atomic<int> error_events{0};
  std::atomic<int> connecting_events{0};
  serial->on_state([&](common::LinkState state) {
    if (state == common::LinkState::Error) error_events.fetch_add(1);
    if (state == common::LinkState::Connecting) connecting_events.fetch_add(1);
  });

  serial->on_bytes([](const uint8_t*, size_t) { throw std::runtime_error("boom"); });

  serial->start();
  ioc.run_for(5ms);

  port_raw->emit_read(4);

  // Allow retry timer to fire at least once.
  ioc.run_for(40ms);

  EXPECT_EQ(error_events.load(), 0);
  EXPECT_GE(connecting_events.load(), 2);  // initial start + retry attempt
  serial->stop();
}

TEST(TransportSerialTest, BackpressureReliefAfterDrain) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.backpressure_threshold = 1024;

  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  std::vector<size_t> events;
  serial->on_backpressure([&](size_t queued) { events.push_back(queued); });

  serial->start();

  std::vector<uint8_t> payload(cfg.backpressure_threshold * 2, 0x11);  // exceed high watermark, below limit
  serial->async_write_copy(payload.data(), payload.size());

  ioc.run_for(50ms);

  ASSERT_GE(events.size(), 2u);
  EXPECT_GE(events.front(), cfg.backpressure_threshold);
  EXPECT_LE(events.back(), cfg.backpressure_threshold / 2);

  serial->stop();
}

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

#include "test/utils/channel_contract_test_utils.hpp"
#include "unilink/common/constants.hpp"
#include "unilink/config/serial_config.hpp"
#include "unilink/interface/iserial_port.hpp"
#include "unilink/transport/serial/serial.hpp"

using namespace std::chrono_literals;
using namespace unilink;
using namespace unilink::transport;
namespace net = boost::asio;

namespace {

// Minimal fake serial port to avoid real device access in tests
class FakeSerialPort : public interface::SerialPortInterface {
 public:
  explicit FakeSerialPort(net::io_context& ioc) : ioc_(ioc) {}

  void open(const std::string&, boost::system::error_code& ec) override {
    open_ = true;
    ec.clear();
  }
  bool is_open() const override { return open_; }
  void close(boost::system::error_code& ec) override {
    open_ = false;
    ec.clear();
  }

  void set_option(const net::serial_port_base::baud_rate&, boost::system::error_code& ec) override { ec.clear(); }
  void set_option(const net::serial_port_base::character_size&, boost::system::error_code& ec) override { ec.clear(); }
  void set_option(const net::serial_port_base::stop_bits&, boost::system::error_code& ec) override { ec.clear(); }
  void set_option(const net::serial_port_base::parity&, boost::system::error_code& ec) override { ec.clear(); }
  void set_option(const net::serial_port_base::flow_control&, boost::system::error_code& ec) override { ec.clear(); }

  void async_read_some(const boost::asio::mutable_buffer&,
                       std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    read_handler_ = std::move(handler);
  }

  void async_write(const boost::asio::const_buffer& buffer,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    auto size = buffer.size();
    net::post(ioc_, [handler = std::move(handler), size]() { handler({}, size); });
  }

  void emit_read(std::size_t n = 1, const boost::system::error_code& ec = {}) {
    if (!read_handler_) return;
    auto handler = std::move(read_handler_);
    net::post(ioc_, [handler = std::move(handler), ec, n]() { handler(ec, n); });
  }

  void emit_operation_aborted() {
    emit_read(0, make_error_code(net::error::operation_aborted));
  }

 private:
  net::io_context& ioc_;
  bool open_{false};
  std::function<void(const boost::system::error_code&, std::size_t)> read_handler_;
};

}  // namespace

namespace unilink {
namespace transport {

// --- Serial contract tests (unit, no real devices) ---

TEST(SerialContractTest, StopIsIdempotent) {
  net::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "";  // ensure FakeSerialPort path is taken
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  test::CallbackRecorder rec;
  serial->on_state(rec.state_cb());

  serial->start();
  test::pump_io(ioc, 10ms);
  serial->stop();
  serial->stop();
  test::pump_io(ioc, 10ms);

  EXPECT_EQ(rec.state_count(common::LinkState::Closed), 1u);
}

TEST(SerialContractTest, NoUserCallbackAfterStop) {
  net::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "";
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto* port_raw = port.get();
  auto serial = Serial::create(cfg, std::move(port), ioc);

  test::CallbackRecorder rec;
  serial->on_bytes(rec.bytes_cb());

  serial->start();
  test::pump_io(ioc, 5ms);
  serial->stop();
  port_raw->emit_operation_aborted();

  EXPECT_FALSE(test::wait_until(ioc, [&] { return rec.bytes_call_count() > 0; }, 100ms));
}

TEST(SerialContractTest, ErrorNotifyOnlyOnce) {
  net::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "";
  cfg.reopen_on_error = false;
  cfg.backpressure_threshold = 512;
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  test::CallbackRecorder rec;
  serial->on_state(rec.state_cb());
  serial->start();

  std::vector<uint8_t> huge(common::constants::DEFAULT_BACKPRESSURE_THRESHOLD * 2, 0xEF);
  serial->async_write_copy(huge.data(), huge.size());

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Error) == 1u; }, 200ms));
  EXPECT_EQ(rec.state_count(common::LinkState::Error), 1u);
}

TEST(SerialContractTest, CallbacksAreSerialized) {
  net::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "";
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto* port_raw = port.get();
  auto serial = Serial::create(cfg, std::move(port), ioc);

  test::CallbackRecorder rec;
  serial->on_bytes(rec.bytes_cb());

  serial->start();
  test::pump_io(ioc, 5ms);

  port_raw->emit_read(4);
  test::pump_io(ioc, 5ms);  // allow re-arm
  port_raw->emit_read(6);

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.bytes_call_count() >= 2; }, 200ms));
  EXPECT_FALSE(rec.saw_overlap());
}

TEST(SerialContractTest, BackpressurePolicyFailFast) {
  net::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "";
  cfg.backpressure_threshold = 256;
  cfg.reopen_on_error = false;
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  test::CallbackRecorder rec;
  serial->on_state(rec.state_cb());
  serial->start();

  std::vector<uint8_t> huge(common::constants::DEFAULT_BACKPRESSURE_THRESHOLD * 2, 0xCD);
  serial->async_write_copy(huge.data(), huge.size());

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Error) == 1u; }, 200ms));
}

TEST(SerialContractTest, OpenCloseLifecycle) {
  net::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "";
  auto port = std::make_unique<FakeSerialPort>(ioc);
  auto serial = Serial::create(cfg, std::move(port), ioc);

  test::CallbackRecorder rec;
  serial->on_state(rec.state_cb());

  serial->start();
  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Connected) == 1u; }, 200ms));
  serial->stop();
  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Closed) == 1u; }, 200ms));
}

}  // namespace transport
}  // namespace unilink

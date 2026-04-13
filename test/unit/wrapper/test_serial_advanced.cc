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
#include <string>
#include <thread>

#include "test/utils/test_utils.hpp"
#include "unilink/config/serial_config.hpp"
#include "unilink/interface/iserial_port.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

namespace {

class FakeSerialPort : public interface::SerialPortInterface {
 public:
  explicit FakeSerialPort(boost::asio::io_context& ioc, boost::system::error_code open_ec = {})
      : ioc_(ioc), open_ec_(open_ec) {}

  void open(const std::string&, boost::system::error_code& ec) override {
    ec = open_ec_;
    open_ = !ec;
  }

  bool is_open() const override { return open_; }

  void close(boost::system::error_code& ec) override {
    open_ = false;
    ec.clear();
    emit_operation_aborted();
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
    auto size = buffer.size();
    boost::asio::post(ioc_, [handler = std::move(handler), size]() { handler({}, size); });
  }

 private:
  void emit_operation_aborted() {
    if (!read_handler_) return;
    auto handler = std::move(read_handler_);
    boost::asio::post(
        ioc_, [handler = std::move(handler)]() { handler(make_error_code(boost::asio::error::operation_aborted), 0); });
  }

  boost::asio::io_context& ioc_;
  boost::system::error_code open_ec_;
  bool open_{false};
  std::function<void(const boost::system::error_code&, std::size_t)> read_handler_;
};

}  // namespace

class SerialWrapperAdvancedTest : public ::testing::Test {
 protected:
  void SetUp() override {
#ifdef _WIN32
    device_ = "NUL";
#else
    device_ = "/dev/null";
#endif
  }

  std::string device_;
};

TEST_F(SerialWrapperAdvancedTest, AutoManageStartsAndStopsChannel) {
  auto serial = unilink::serial(device_, 9600).auto_manage(true).build();

  std::atomic<bool> connected{false};
  std::atomic<bool> disconnected{false};

  serial->on_connect([&](const wrapper::ConnectionContext&) { connected = true; });
  serial->on_disconnect([&](const wrapper::ConnectionContext&) { disconnected = true; });

  // In auto-manage mode, start() is called automatically
  TestUtils::waitFor(100);

  serial->stop();
  EXPECT_TRUE(disconnected.load() || !serial->is_connected());
}

TEST_F(SerialWrapperAdvancedTest, ConfigurationSetters) {
  auto serial = std::make_shared<wrapper::Serial>(device_, 9600);

  serial->set_baud_rate(115200);
  serial->set_data_bits(7);
  serial->set_stop_bits(2);
  serial->set_parity("even");
  serial->set_flow_control("hardware");
  serial->set_retry_interval(500ms);

  // Should be able to start with these settings
  serial->start();
  serial->stop();
}

TEST_F(SerialWrapperAdvancedTest, AutoManageStartsInjectedTransport) {
  boost::asio::io_context ioc;

  config::SerialConfig cfg;
  cfg.device = "fake";
  cfg.baud_rate = 9600;
  cfg.reopen_on_error = false;

  auto transport_serial = transport::Serial::create(cfg, std::make_unique<FakeSerialPort>(ioc), ioc);
  wrapper::Serial serial(std::static_pointer_cast<interface::Channel>(transport_serial));

  serial.auto_manage(true);
  ioc.run_for(50ms);

  EXPECT_TRUE(serial.is_connected());

  serial.stop();
}

TEST_F(SerialWrapperAdvancedTest, StartFutureReflectsTransportFailure) {
  boost::asio::io_context ioc;

  config::SerialConfig cfg;
  cfg.device = "fake";
  cfg.baud_rate = 9600;
  cfg.reopen_on_error = false;

  auto transport_serial = transport::Serial::create(
      cfg, std::make_unique<FakeSerialPort>(ioc, make_error_code(boost::asio::error::access_denied)), ioc);
  wrapper::Serial serial(std::static_pointer_cast<interface::Channel>(transport_serial));

  auto started = serial.start();
  ioc.run_for(50ms);

  ASSERT_EQ(started.wait_for(100ms), std::future_status::ready);
  EXPECT_FALSE(started.get());
  EXPECT_FALSE(serial.is_connected());

  serial.stop();
}

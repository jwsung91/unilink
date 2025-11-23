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

#include "unilink/config/serial_config.hpp"
#include "unilink/interface/iserial_port.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/wrapper/serial/serial.hpp"

using namespace unilink;
using namespace unilink::wrapper;
using namespace std::chrono_literals;

namespace {

// Fake port that records options set through the Serial wrapper mapping
class RecordingSerialPort : public interface::SerialPortInterface {
 public:
  explicit RecordingSerialPort(boost::asio::io_context& ioc) : ioc_(ioc) {}

  void open(const std::string& device, boost::system::error_code& ec) override {
    device_ = device;
    open_ = true;
    ec.clear();
  }
  bool is_open() const override { return open_; }
  void close(boost::system::error_code& ec) override {
    open_ = false;
    ec.clear();
  }

  void set_option(const boost::asio::serial_port_base::baud_rate& option, boost::system::error_code& ec) override {
    baud_rate_ = option.value();
    ec.clear();
  }
  void set_option(const boost::asio::serial_port_base::character_size& option, boost::system::error_code& ec) override {
    char_size_ = option.value();
    ec.clear();
  }
  void set_option(const boost::asio::serial_port_base::stop_bits& option, boost::system::error_code& ec) override {
    stop_bits_ = option.value();
    ec.clear();
  }
  void set_option(const boost::asio::serial_port_base::parity& option, boost::system::error_code& ec) override {
    parity_ = option.value();
    ec.clear();
  }
  void set_option(const boost::asio::serial_port_base::flow_control& option, boost::system::error_code& ec) override {
    flow_ = option.value();
    ec.clear();
  }

  void async_read_some(const boost::asio::mutable_buffer&,
                       std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    handler_ = std::move(handler);
  }
  void async_write(const boost::asio::const_buffer&,
                   std::function<void(const boost::system::error_code&, std::size_t)> handler) override {
    boost::system::error_code ok;
    boost::asio::post(ioc_, [handler = std::move(handler), ok]() { handler(ok, 0); });
  }

  void emit_operation_aborted() {
    if (!handler_) return;
    auto cb = std::move(handler_);
    boost::asio::post(ioc_, [cb]() { cb(make_error_code(boost::asio::error::operation_aborted), 0); });
  }

  // Recorded values
  std::string device_;
  unsigned baud_rate_{0};
  unsigned char_size_{0};
  unsigned stop_bits_{0};
  boost::asio::serial_port_base::parity::type parity_{boost::asio::serial_port_base::parity::none};
  boost::asio::serial_port_base::flow_control::type flow_{boost::asio::serial_port_base::flow_control::none};

 private:
  boost::asio::io_context& ioc_;
  bool open_{false};
  std::function<void(const boost::system::error_code&, std::size_t)> handler_;
};

}  // namespace

TEST(SerialConfigMappingTest, MapsParityAndFlowFromStrings) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "/dev/ttyS10";
  cfg.baud_rate = 57600;
  cfg.read_chunk = 8;  // keep small for test

  auto port = std::make_unique<RecordingSerialPort>(ioc);
  auto* port_raw = port.get();

  Serial wrapper(cfg.device, cfg.baud_rate);
  wrapper.set_data_bits(7);
  wrapper.set_stop_bits(2);
  wrapper.set_parity("Even");
  wrapper.set_flow_control("hardware");

  auto serial = std::make_shared<transport::Serial>(cfg, std::move(port), ioc);
  serial->start();
  ioc.run_for(5ms);
  serial->stop();

  EXPECT_EQ(port_raw->device_, cfg.device);
  EXPECT_EQ(port_raw->baud_rate_, cfg.baud_rate);
  EXPECT_EQ(port_raw->char_size_, 7u);
  EXPECT_EQ(port_raw->stop_bits_, 2u);
  EXPECT_EQ(port_raw->parity_, boost::asio::serial_port_base::parity::even);
  EXPECT_EQ(port_raw->flow_, boost::asio::serial_port_base::flow_control::hardware);
}

TEST(SerialConfigMappingTest, InvalidStringsFallbackToNone) {
  boost::asio::io_context ioc;
  config::SerialConfig cfg;
  cfg.device = "/dev/ttyS11";

  auto port = std::make_unique<RecordingSerialPort>(ioc);
  auto* port_raw = port.get();

  Serial wrapper(cfg.device, cfg.baud_rate);
  wrapper.set_parity("invalid");
  wrapper.set_flow_control("???");

  auto serial = std::make_shared<transport::Serial>(cfg, std::move(port), ioc);
  serial->start();
  ioc.run_for(5ms);
  serial->stop();

  EXPECT_EQ(port_raw->parity_, boost::asio::serial_port_base::parity::none);
  EXPECT_EQ(port_raw->flow_, boost::asio::serial_port_base::flow_control::none);
}

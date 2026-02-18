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

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "unilink/unilink.hpp"

using namespace unilink;

class BuilderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_port_ = 8080;
    data_received_.clear();
    connection_established_ = false;
    error_occurred_ = false;
  }

  void setupDataHandler() {
    auto data_cb = [this](const wrapper::MessageContext& ctx) { 
      data_received_.push_back(std::string(ctx.data())); 
    };
    if (server_) server_->on_data(data_cb);
    if (client_) client_->on_data(data_cb);
    if (serial_) serial_->on_data(data_cb);
  }

  void setupConnectionHandler() {
    auto conn_cb = [this](const wrapper::ConnectionContext&) { 
      connection_established_ = true; 
    };
    if (server_) server_->on_client_connect(conn_cb);
    if (client_) client_->on_connect(conn_cb);
    if (serial_) serial_->on_connect(conn_cb);
  }

  void setupErrorHandler() {
    auto err_cb = [this](const wrapper::ErrorContext& ctx) {
      error_occurred_ = true;
      last_error_ = std::string(ctx.message());
    };
    if (server_) server_->on_error(err_cb);
    if (client_) client_->on_error(err_cb);
    if (serial_) serial_->on_error(err_cb);
  }

  std::string nullDevice() const {
#ifdef _WIN32
    return "NUL";
#else
    return "/dev/null";
#endif
  }

  uint16_t test_port_;
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;
  std::shared_ptr<wrapper::Serial> serial_;
  std::shared_ptr<wrapper::Udp> udp_;

  std::vector<std::string> data_received_;
  bool connection_established_;
  bool error_occurred_;
  std::string last_error_;
};

TEST_F(BuilderTest, TcpServerBuilderBasic) {
  server_ = tcp_server(test_port_)
                .single_client()
                .on_data([](const wrapper::MessageContext& ctx) {
                  // 데이터 처리
                })
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_FALSE(server_->is_listening());
}

TEST_F(BuilderTest, TcpClientBuilderBasic) {
  client_ = tcp_client("127.0.0.1", test_port_)
                .retry_interval(1000)
                .on_data([](const wrapper::MessageContext& ctx) {
                  // 데이터 처리
                })
                .build();

  ASSERT_NE(client_, nullptr);
  EXPECT_FALSE(client_->is_connected());
}

TEST_F(BuilderTest, SerialBuilderBasic) {
  serial_ = serial(nullDevice(), 9600)
                .data_bits(8)
                .on_data([](const wrapper::MessageContext& ctx) {
                  // 데이터 처리
                })
                .build();

  ASSERT_NE(serial_, nullptr);
  EXPECT_FALSE(serial_->is_connected());
}

TEST_F(BuilderTest, UdpBuilderBasic) {
  udp_ = udp(test_port_)
             .set_remote("127.0.0.1", 9000)
             .build();

  ASSERT_NE(udp_, nullptr);
}

TEST_F(BuilderTest, BuilderChaining) {
  server_ = tcp_server(test_port_)
                .unlimited_clients()
                .enable_port_retry(true, 5, 500)
                .on_data([this](const wrapper::MessageContext& ctx) { 
                  data_received_.push_back(std::string(ctx.data())); 
                })
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_FALSE(server_->is_listening());
}

TEST_F(BuilderTest, MultipleBuilders) {
  auto server_builder = tcp_server(test_port_);
  auto client_builder = tcp_client("localhost", test_port_);

  server_ = server_builder.build();
  client_ = client_builder.build();

  EXPECT_NE(server_, nullptr);
  EXPECT_NE(client_, nullptr);
}

TEST_F(BuilderTest, BuilderConfiguration) {
  server_ = tcp_server(test_port_)
                .idle_timeout(5000)
                .max_clients(10)
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_FALSE(server_->is_listening());
}

TEST_F(BuilderTest, CallbackRegistration) {
  int callback_count = 0;
  server_ = tcp_server(test_port_)
                .on_data([&callback_count](const wrapper::MessageContext& ctx) { callback_count++; })
                .build();

  ASSERT_NE(server_, nullptr);
}

TEST_F(BuilderTest, BuilderReuse) {
  builder::TcpServerBuilder builder(test_port_);

  auto server1 = builder.unlimited_clients().on_data([](const wrapper::MessageContext& ctx) {}).build();
  ASSERT_NE(server1, nullptr);

  auto server2 = builder.on_connect([](const wrapper::ConnectionContext& ctx) {}).build();
  ASSERT_NE(server2, nullptr);
}

TEST_F(BuilderTest, ConvenienceFunctions) {
  auto server = unilink::tcp_server(test_port_)
                    .on_connect([](const wrapper::ConnectionContext& ctx) {})
                    .build();
  EXPECT_NE(server, nullptr);

  auto client =
      unilink::tcp_client("127.0.0.1", test_port_).on_connect([](const wrapper::ConnectionContext& ctx) {}).on_data([](const wrapper::MessageContext& ctx) {}).build();
  EXPECT_NE(client, nullptr);

  auto serial = unilink::serial(nullDevice(), 9600).on_connect([](const wrapper::ConnectionContext& ctx) {}).on_data([](const wrapper::MessageContext& ctx) {}).build();
  EXPECT_NE(serial, nullptr);

  auto udp = unilink::udp(test_port_)
                 .on_connect([](const wrapper::ConnectionContext& ctx) {})
                 .build();
  EXPECT_NE(udp, nullptr);
}

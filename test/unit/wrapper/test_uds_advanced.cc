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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <string_view>

#include "test/mocks/mock_uds_acceptor.hpp"
#include "test/mocks/mock_uds_socket.hpp"
#include "test_utils.hpp"
#include "unilink/interface/channel.hpp"
#include "unilink/transport/uds/uds_client.hpp"
#include "unilink/transport/uds/uds_server.hpp"
#include "unilink/wrapper/uds_client/uds_client.hpp"
#include "unilink/wrapper/uds_server/uds_server.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using namespace std::chrono_literals;

namespace unilink::wrapper {
namespace {

class FakeChannel : public interface::Channel {
 public:
  void start() override { connected_ = true; }
  void stop() override { connected_ = false; }
  bool is_connected() const override { return connected_; }

  void async_write_copy(memory::ConstByteSpan) override {}
  void async_write_move(std::vector<uint8_t>&&) override {}
  void async_write_shared(std::shared_ptr<const std::vector<uint8_t>>) override {}

  void on_bytes(OnBytes cb) override { on_bytes_ = std::move(cb); }
  void on_state(OnState cb) override { on_state_ = std::move(cb); }
  void on_backpressure(OnBackpressure cb) override { on_backpressure_ = std::move(cb); }

  void emit_bytes(std::string_view text) {
    if (!on_bytes_) return;
    on_bytes_(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(text.data()), text.size()));
  }

  void emit_state(base::LinkState state) {
    if (state == base::LinkState::Connected) {
      connected_ = true;
    } else if (state == base::LinkState::Closed || state == base::LinkState::Error || state == base::LinkState::Idle) {
      connected_ = false;
    }

    if (on_state_) {
      on_state_(state);
    }
  }

 private:
  bool connected_{false};
  OnBytes on_bytes_;
  OnState on_state_;
  OnBackpressure on_backpressure_;
};

TEST(UdsClientWrapperAdvancedTest, AutoManageStartsInjectedTransport) {
  boost::asio::io_context ioc;
  config::UdsClientConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uwc").string();

  auto* mock_socket = new test::mocks::MockUdsSocket();
  auto transport_client =
      transport::UdsClient::create(cfg, std::unique_ptr<interface::UdsSocketInterface>(mock_socket), ioc);

  EXPECT_CALL(*mock_socket, async_connect(_, _)).WillOnce(Invoke([&ioc](const auto&, auto handler) {
    boost::asio::post(ioc, [handler]() { handler({}); });
  }));
  EXPECT_CALL(*mock_socket, async_read_some(_, _)).WillRepeatedly(Invoke([](const auto&, auto) {}));

  UdsClient client(std::static_pointer_cast<interface::Channel>(transport_client));
  client.auto_manage(true);

  ioc.restart();
  ioc.run_for(100ms);

  EXPECT_TRUE(client.is_connected());

  client.stop();
  ioc.restart();
  ioc.run_for(50ms);
}

TEST(UdsClientWrapperAdvancedTest, StartFutureReflectsTransportFailure) {
  boost::asio::io_context ioc;
  config::UdsClientConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uwc-fail").string();

  auto* mock_socket = new test::mocks::MockUdsSocket();
  auto transport_client =
      transport::UdsClient::create(cfg, std::unique_ptr<interface::UdsSocketInterface>(mock_socket), ioc);

  EXPECT_CALL(*mock_socket, async_connect(_, _)).WillOnce(Invoke([&ioc](const auto&, auto handler) {
    boost::asio::post(ioc, [handler]() { handler(make_error_code(boost::asio::error::connection_refused)); });
  }));

  UdsClient client(std::static_pointer_cast<interface::Channel>(transport_client));
  auto started = client.start();

  ioc.restart();
  ioc.run_for(100ms);

  ASSERT_EQ(started.wait_for(0ms), std::future_status::ready);
  EXPECT_FALSE(started.get());

  client.stop();
  ioc.restart();
  ioc.run_for(50ms);
}

TEST(UdsClientWrapperAdvancedTest, ManagedExternalContextStopsOnShutdown) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  auto socket_path = test::TestUtils::makeUniqueUdsSocketPath("uwc-managed").string();
  test::TestUtils::removeFileIfExists(socket_path);

  UdsServer server(socket_path);
  auto server_started = server.start();
  ASSERT_EQ(server_started.wait_for(1s), std::future_status::ready);
  ASSERT_TRUE(server_started.get());

  UdsClient client(socket_path, ioc);
  client.set_manage_external_context(true);

  ioc->stop();
  auto started = client.start();
  ASSERT_EQ(started.wait_for(1s), std::future_status::ready);
  EXPECT_TRUE(started.get());

  EXPECT_TRUE(unilink::test::TestUtils::waitForCondition([&]() { return client.is_connected(); }, 2000));

  client.stop();
  EXPECT_TRUE(ioc->stopped());

  server.stop();
  test::TestUtils::removeFileIfExists(socket_path);
}

TEST(UdsServerWrapperAdvancedTest, AutoManageStartsInjectedTransport) {
  boost::asio::io_context ioc;
  config::UdsServerConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uws").string();
  test::TestUtils::removeFileIfExists(cfg.socket_path);

  auto* mock_acceptor = new test::mocks::MockUdsAcceptor();
  auto transport_server =
      transport::UdsServer::create(cfg, std::unique_ptr<interface::UdsAcceptorInterface>(mock_acceptor), ioc);

  EXPECT_CALL(*mock_acceptor, open(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, bind(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, listen(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, async_accept(_)).WillOnce(Invoke([](auto) {}));

  UdsServer server(std::static_pointer_cast<interface::Channel>(transport_server));
  server.auto_manage(true);

  ioc.restart();
  ioc.poll();

  EXPECT_TRUE(server.is_listening());

  server.stop();
  ioc.restart();
  ioc.run_for(50ms);
}

TEST(UdsServerWrapperAdvancedTest, StartFutureReflectsBindFailure) {
  boost::asio::io_context ioc;
  config::UdsServerConfig cfg;
  cfg.socket_path = test::TestUtils::makeUniqueUdsSocketPath("uws-fail").string();
  test::TestUtils::removeFileIfExists(cfg.socket_path);

  auto* mock_acceptor = new test::mocks::MockUdsAcceptor();
  auto transport_server =
      transport::UdsServer::create(cfg, std::unique_ptr<interface::UdsAcceptorInterface>(mock_acceptor), ioc);

  EXPECT_CALL(*mock_acceptor, open(_, _)).WillOnce(Return());
  EXPECT_CALL(*mock_acceptor, bind(_, _)).WillOnce(Invoke([](const auto&, boost::system::error_code& ec) {
    ec = make_error_code(boost::asio::error::address_in_use);
  }));

  UdsServer server(std::static_pointer_cast<interface::Channel>(transport_server));
  auto started = server.start();

  ioc.restart();
  ioc.poll();

  ASSERT_EQ(started.wait_for(0ms), std::future_status::ready);
  EXPECT_FALSE(started.get());
  EXPECT_FALSE(server.is_listening());

  server.stop();
  ioc.restart();
  ioc.run_for(50ms);
}

TEST(UdsServerWrapperAdvancedTest, ManagedExternalContextStopsOnShutdown) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  auto socket_path = test::TestUtils::makeUniqueUdsSocketPath("uws-managed").string();
  test::TestUtils::removeFileIfExists(socket_path);

  UdsServer server(socket_path, ioc);
  server.set_manage_external_context(true);

  ioc->stop();
  auto started = server.start();
  ASSERT_EQ(started.wait_for(1s), std::future_status::ready);
  EXPECT_TRUE(started.get());

  EXPECT_TRUE(unilink::test::TestUtils::waitForCondition([&]() { return server.is_listening(); }, 2000));

  server.stop();
  EXPECT_TRUE(ioc->stopped());
  test::TestUtils::removeFileIfExists(socket_path);
}

TEST(UdsClientWrapperContractTest, HandlerReplacementUsesLatestCallback) {
  auto fake_channel = std::make_shared<FakeChannel>();
  UdsClient client(fake_channel);

  std::atomic<int> connected{0};
  std::atomic<int> data{0};
  std::atomic<int> errors{0};

  client.on_connect([&](const ConnectionContext&) { connected = 1; });
  client.on_connect([&](const ConnectionContext&) { connected = 2; });

  client.on_data([&](const MessageContext&) { data = 1; });
  client.on_data([&](const MessageContext&) { data = 2; });

  client.on_error([&](const ErrorContext&) { errors = 1; });
  client.on_error([&](const ErrorContext&) { errors = 2; });

  fake_channel->emit_state(base::LinkState::Connected);
  fake_channel->emit_bytes("payload");
  fake_channel->emit_state(base::LinkState::Error);

  EXPECT_EQ(connected.load(), 2);
  EXPECT_EQ(data.load(), 2);
  EXPECT_EQ(errors.load(), 2);
}

TEST(UdsClientWrapperContractTest, StopSuppressesLateCallbacks) {
  auto fake_channel = std::make_shared<FakeChannel>();
  UdsClient client(fake_channel);

  std::atomic<int> callbacks{0};
  client.on_connect([&](const ConnectionContext&) { callbacks++; });
  client.on_data([&](const MessageContext&) { callbacks++; });
  client.on_error([&](const ErrorContext&) { callbacks++; });
  client.on_disconnect([&](const ConnectionContext&) { callbacks++; });

  client.start();
  client.stop();

  fake_channel->emit_state(base::LinkState::Connected);
  fake_channel->emit_bytes("payload");
  fake_channel->emit_state(base::LinkState::Error);
  fake_channel->emit_state(base::LinkState::Closed);

  EXPECT_EQ(callbacks.load(), 0);
}

}  // namespace
}  // namespace unilink::wrapper

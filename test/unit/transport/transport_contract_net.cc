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
#include <stdexcept>
#include <string>
#include <vector>

#include "test/utils/channel_contract_test_utils.hpp"
#include "unilink/common/common.hpp"
#include "unilink/common/constants.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/udp/udp.hpp"

using namespace std::chrono_literals;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

uint16_t reserve_udp_port() {
  net::io_context ioc;
  boost::system::error_code ec;
  net::ip::udp::socket socket(ioc);
  socket.open(net::ip::udp::v4(), ec);
  if (ec) throw std::runtime_error("udp open failed: " + ec.message());
  socket.bind({net::ip::udp::v4(), 0}, ec);
  if (ec) throw std::runtime_error("udp bind failed: " + ec.message());
  auto port = socket.local_endpoint().port();
  socket.close();
  return port;
}

bool can_bind_udp() {
  try {
    reserve_udp_port();
    return true;
  } catch (...) {
    return false;
  }
}

bool can_bind_tcp() {
  try {
    net::io_context ioc;
    tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
    (void)acceptor.local_endpoint();
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace

namespace unilink {
namespace transport {

// --- UDP contract tests (network/loopback) ---

TEST(UdpContractTest, StopIsIdempotent) {
  if (!can_bind_udp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = reserve_udp_port();
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = static_cast<uint16_t>(cfg.local_port + 1);

  auto channel = UdpChannel::create(cfg, ioc);
  test::CallbackRecorder rec;
  channel->on_state(rec.state_cb());

  channel->start();
  test::pump_io(ioc, 20ms);

  channel->stop();
  channel->stop();
  test::pump_io(ioc, 20ms);

  EXPECT_EQ(rec.state_count(common::LinkState::Closed), 1u);
}

TEST(UdpContractTest, NoUserCallbackAfterStop) {
  if (!can_bind_udp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = reserve_udp_port();

  auto channel = UdpChannel::create(cfg, ioc);
  test::CallbackRecorder rec;
  channel->on_bytes(rec.bytes_cb());

  channel->start();
  test::pump_io(ioc, 20ms);
  channel->stop();

  net::ip::udp::socket peer(ioc, {net::ip::udp::v4(), 0});
  net::ip::udp::endpoint ep{net::ip::make_address("127.0.0.1"), cfg.local_port};
  peer.send_to(net::buffer(std::string("after-stop")), ep);

  EXPECT_FALSE(test::wait_until(ioc, [&] { return rec.bytes_call_count() > 0; }, 100ms));
}

TEST(UdpContractTest, ErrorNotifyOnlyOnce) {
  if (!can_bind_udp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  uint16_t port = reserve_udp_port();

  config::UdpConfig cfg;
  cfg.local_port = port;
  auto channel = UdpChannel::create(cfg, ioc);

  test::CallbackRecorder rec;
  channel->on_state(rec.state_cb());
  channel->start();
  test::pump_io(ioc, 20ms);  // allow bind + receive arm

  net::ip::udp::socket peer(ioc, {net::ip::udp::v4(), 0});
  net::ip::udp::endpoint ep{net::ip::make_address("127.0.0.1"), port};

  std::vector<uint8_t> big(common::constants::DEFAULT_READ_BUFFER_SIZE + 256, 0xAB);
  peer.send_to(net::buffer(big), ep);

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Error) == 1u; }, 200ms));
  EXPECT_EQ(rec.state_count(common::LinkState::Error), 1u);
}

TEST(UdpContractTest, CallbacksAreSerialized) {
  if (!can_bind_udp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  uint16_t port = reserve_udp_port();

  config::UdpConfig cfg;
  cfg.local_port = port;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = static_cast<uint16_t>(port + 1);

  auto channel = UdpChannel::create(cfg, ioc);
  test::CallbackRecorder rec;
  channel->on_bytes(rec.bytes_cb());

  net::ip::udp::socket peer(ioc, {net::ip::udp::v4(), 0});
  net::ip::udp::endpoint ep{net::ip::make_address("127.0.0.1"), cfg.local_port};

  channel->start();
  test::pump_io(ioc, 20ms);

  peer.send_to(net::buffer(std::string("one")), ep);
  peer.send_to(net::buffer(std::string("two")), ep);

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.bytes_call_count() >= 2; }, 200ms));
  EXPECT_FALSE(rec.saw_overlap());
}

TEST(UdpContractTest, BackpressurePolicyFailFast) {
  if (!can_bind_udp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  uint16_t base_port = reserve_udp_port();

  config::UdpConfig cfg;
  cfg.local_port = base_port;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = static_cast<uint16_t>(base_port + 1);
  cfg.backpressure_threshold = common::constants::MIN_BACKPRESSURE_THRESHOLD;

  auto channel = UdpChannel::create(cfg, ioc);
  test::CallbackRecorder rec;
  channel->on_state(rec.state_cb());
  channel->start();

  std::vector<uint8_t> huge(common::constants::DEFAULT_BACKPRESSURE_THRESHOLD * 2, 0xCD);
  channel->async_write_copy(huge.data(), huge.size());

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Error) == 1u; }, 200ms));
}

TEST(UdpContractTest, OpenCloseLifecycle) {
  if (!can_bind_udp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  uint16_t base_port = reserve_udp_port();

  config::UdpConfig sender_cfg;
  sender_cfg.local_port = base_port;
  sender_cfg.remote_address = "127.0.0.1";
  sender_cfg.remote_port = static_cast<uint16_t>(base_port + 1);

  config::UdpConfig receiver_cfg;
  receiver_cfg.local_port = static_cast<uint16_t>(base_port + 1);
  receiver_cfg.remote_address = "127.0.0.1";
  receiver_cfg.remote_port = base_port;

  auto sender = UdpChannel::create(sender_cfg, ioc);
  auto receiver = UdpChannel::create(receiver_cfg, ioc);
  test::CallbackRecorder rec;
  receiver->on_state(rec.state_cb());

  receiver->start();
  sender->start();

  auto payload = common::safe_convert::string_to_uint8("ping");
  sender->async_write_copy(payload.data(), payload.size());

  EXPECT_TRUE(test::wait_until(ioc, [&] { return receiver->is_connected(); }, 200ms));
  receiver->stop();
  sender->stop();

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Closed) == 1u; }, 200ms));
}

TEST(UdpContractTest, WriteWithoutRemoteIsDocumentedNoop) {
  if (!can_bind_udp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = reserve_udp_port();

  auto channel = UdpChannel::create(cfg, ioc);
  test::CallbackRecorder rec;
  channel->on_state(rec.state_cb());
  channel->start();

  auto data = common::safe_convert::string_to_uint8("orphan");
  channel->async_write_copy(data.data(), data.size());

  EXPECT_FALSE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Error) > 0; }, 100ms));
  channel->stop();
}

// --- TCP client contract tests (network/loopback) ---

TEST(TcpContractTest, StopIsIdempotent) {
  if (!can_bind_tcp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
  auto port = acceptor.local_endpoint().port();
  std::shared_ptr<tcp::socket> server_socket = std::make_shared<tcp::socket>(ioc);
  acceptor.async_accept(*server_socket, [](const boost::system::error_code& ec) { ASSERT_FALSE(ec); });

  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = port;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg, ioc);
  test::CallbackRecorder rec;
  client->on_state(rec.state_cb());

  client->start();
  test::pump_io(ioc, 50ms);
  client->stop();
  client->stop();
  test::pump_io(ioc, 50ms);

  EXPECT_EQ(rec.state_count(common::LinkState::Closed), 1u);
}

TEST(TcpContractTest, NoUserCallbackAfterStop) {
  if (!can_bind_tcp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
  auto port = acceptor.local_endpoint().port();
  auto server_socket = std::make_shared<tcp::socket>(ioc);
  acceptor.async_accept(*server_socket, [](const boost::system::error_code& ec) { ASSERT_FALSE(ec); });

  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = port;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg, ioc);
  test::CallbackRecorder rec;
  client->on_bytes(rec.bytes_cb());

  client->start();
  EXPECT_TRUE(test::wait_until(ioc, [&] { return client->is_connected(); }, 200ms));

  auto data1 = std::make_shared<std::string>("before-stop");
  net::async_write(*server_socket, net::buffer(*data1), [data1](auto, auto) {});
  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.bytes_call_count() >= 1; }, 200ms));

  client->stop();
  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Closed) >= 1; }, 200ms));
  test::pump_io(ioc, 50ms);
  auto data2 = std::make_shared<std::string>("after-stop");
  net::async_write(*server_socket, net::buffer(*data2), [data2](auto, auto) {});
  EXPECT_FALSE(test::wait_until(ioc, [&] { return rec.bytes_call_count() > 1; }, 500ms));
}

TEST(TcpContractTest, ErrorNotifyOnlyOnce) {
  if (!can_bind_tcp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 1;  // closed port to avoid real connect
  cfg.backpressure_threshold = 1024;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg, ioc);
  test::CallbackRecorder rec;
  client->on_state(rec.state_cb());

  std::vector<uint8_t> huge(cfg.backpressure_threshold * 4096, 0xAB);
  client->async_write_copy(huge.data(), huge.size());
  client->start();

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Error) == 1u; }, 200ms));
  EXPECT_EQ(rec.state_count(common::LinkState::Error), 1u);
}

TEST(TcpContractTest, CallbacksAreSerialized) {
  if (!can_bind_tcp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
  auto port = acceptor.local_endpoint().port();
  auto server_socket = std::make_shared<tcp::socket>(ioc);
  acceptor.async_accept(*server_socket, [](const boost::system::error_code& ec) { ASSERT_FALSE(ec); });

  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = port;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg, ioc);
  test::CallbackRecorder rec;
  client->on_bytes(rec.bytes_cb());

  client->start();
  ASSERT_TRUE(test::wait_until(ioc, [&] { return client->is_connected(); }, 200ms));

  auto burst = std::make_shared<std::string>("burst-one");
  auto burst2 = std::make_shared<std::string>("burst-two");
  net::async_write(*server_socket, net::buffer(*burst),
                   [burst](const boost::system::error_code& ec, std::size_t) { ASSERT_FALSE(ec); });
  test::pump_io(ioc, 20ms);
  net::async_write(*server_socket, net::buffer(*burst2),
                   [burst2](const boost::system::error_code& ec, std::size_t) { ASSERT_FALSE(ec); });

  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.bytes_call_count() >= 2; }, 1000ms));
  EXPECT_FALSE(rec.saw_overlap());
}

TEST(TcpContractTest, BackpressurePolicyFailFast) {
  if (!can_bind_tcp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 1;  // no real server
  cfg.backpressure_threshold = 1024;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg, ioc);
  test::CallbackRecorder rec;
  client->on_state(rec.state_cb());

  std::vector<uint8_t> huge(cfg.backpressure_threshold * 4096, 0xCD);
  client->async_write_copy(huge.data(), huge.size());

  ioc.run_for(50ms);
  EXPECT_EQ(rec.state_count(common::LinkState::Error), 1u);
}

TEST(TcpContractTest, OpenCloseLifecycle) {
  if (!can_bind_tcp()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  net::io_context ioc;
  tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
  auto port = acceptor.local_endpoint().port();
  auto server_socket = std::make_shared<tcp::socket>(ioc);
  acceptor.async_accept(*server_socket, [](const boost::system::error_code& ec) { ASSERT_FALSE(ec); });

  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = port;
  cfg.max_retries = 0;

  auto client = TcpClient::create(cfg, ioc);
  test::CallbackRecorder rec;
  client->on_state(rec.state_cb());

  client->start();
  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Connected) == 1u; }, 200ms));
  client->stop();
  EXPECT_TRUE(test::wait_until(ioc, [&] { return rec.state_count(common::LinkState::Closed) == 1u; }, 200ms));
}

}  // namespace transport
}  // namespace unilink

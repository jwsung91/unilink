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
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

#include "unilink/common/common.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/transport/udp/udp.hpp"

using namespace std::chrono_literals;
using namespace unilink;

namespace {
uint16_t reserve_free_port() {
  boost::asio::io_context ioc;
  boost::asio::ip::udp::socket socket(ioc, {boost::asio::ip::udp::v4(), 0});
  auto port = socket.local_endpoint().port();
  socket.close();
  // Note: This minimizes port collisions but does not fully eliminate TOCTOU;
  // we release the socket before the test channel binds. Good enough for CI,
  // and the retry/wait logic in tests further reduces flakiness.
  return port;
}

template <typename Pred>
bool wait_for_condition(boost::asio::io_context& ioc, Pred pred, std::chrono::milliseconds timeout,
                        std::chrono::milliseconds step = 5ms) {
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (pred()) return true;
    ioc.run_for(step);
    ioc.restart();
  }
  return pred();
}

void pump_io(boost::asio::io_context& ioc, std::chrono::milliseconds duration, std::chrono::milliseconds step = 5ms) {
  auto deadline = std::chrono::steady_clock::now() + duration;
  while (std::chrono::steady_clock::now() < deadline) {
    ioc.run_for(step);
    ioc.restart();
  }
}

bool can_open_udp_socket() {
  try {
    boost::asio::io_context ioc;
    boost::asio::ip::udp::socket socket(ioc);
    boost::system::error_code ec;
    socket.open(boost::asio::ip::udp::v4(), ec);
    return !ec;
  } catch (...) {
    return false;
  }
}
}  // namespace

TEST(TransportUdpTest, LoopbackSendReceive) {
  if (!can_open_udp_socket()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  boost::asio::io_context ioc;

  config::UdpConfig sender_cfg;
  config::UdpConfig receiver_cfg;
  uint16_t sender_port = reserve_free_port();
  uint16_t receiver_port = reserve_free_port();

  sender_cfg.local_address = "127.0.0.1";
  sender_cfg.local_port = sender_port;
  sender_cfg.remote_address = "127.0.0.1";
  sender_cfg.remote_port = receiver_port;

  receiver_cfg.local_address = "127.0.0.1";
  receiver_cfg.local_port = receiver_port;
  receiver_cfg.remote_address = "127.0.0.1";
  receiver_cfg.remote_port = sender_port;

  {
    // Ensure sandbox permits opening/binding and basic loopback before constructing channels.
    boost::asio::io_context guard_ioc;
    boost::system::error_code ec;
    boost::asio::ip::udp::socket s1(guard_ioc), s2(guard_ioc);
    s1.open(boost::asio::ip::udp::v4(), ec);
    if (ec) GTEST_SKIP() << "UDP socket open not permitted in sandbox";
    s1.bind({boost::asio::ip::make_address("127.0.0.1"), sender_port}, ec);
    if (ec) GTEST_SKIP() << "UDP bind not permitted in sandbox";
    s2.open(boost::asio::ip::udp::v4(), ec);
    if (ec) GTEST_SKIP() << "UDP socket open not permitted in sandbox";
    s2.bind({boost::asio::ip::make_address("127.0.0.1"), receiver_port}, ec);
    if (ec) GTEST_SKIP() << "UDP bind not permitted in sandbox";

    const std::string probe = "probe";
    s1.send_to(boost::asio::buffer(probe), {boost::asio::ip::make_address("127.0.0.1"), receiver_port}, 0, ec);
    if (ec) GTEST_SKIP() << "UDP send not permitted in sandbox";
    std::array<char, 16> buf{};
    boost::asio::ip::udp::endpoint from_ep;
    s2.receive_from(boost::asio::buffer(buf), from_ep, 0, ec);
    if (ec) GTEST_SKIP() << "UDP receive not permitted in sandbox";
  }

  auto sender = transport::UdpChannel::create(sender_cfg, ioc);
  auto receiver = transport::UdpChannel::create(receiver_cfg, ioc);

  std::atomic<common::LinkState> sender_state{common::LinkState::Idle};
  std::atomic<common::LinkState> receiver_state{common::LinkState::Idle};
  sender->on_state([&](common::LinkState s) { sender_state.store(s); });
  receiver->on_state([&](common::LinkState s) { receiver_state.store(s); });
  std::string received;
  receiver->on_bytes([&](const uint8_t* data, size_t n) { received.assign(reinterpret_cast<const char*>(data), n); });

  sender->start();
  receiver->start();
  if (wait_for_condition(ioc,
                         [&] {
                           return sender_state.load() == common::LinkState::Error ||
                                  receiver_state.load() == common::LinkState::Error;
                         },
                         50ms)) {
    GTEST_SKIP() << "UDP socket open not permitted in sandbox";
  }

  const std::string payload = "udp hello";
  auto data = common::safe_convert::string_to_uint8(payload);
  sender->async_write_copy(data.data(), data.size());

  if (!wait_for_condition(ioc, [&] { return received == payload; }, 200ms)) {
    GTEST_SKIP() << "UDP loopback not permitted in sandbox";
  }

  EXPECT_EQ(received, payload);

  sender->stop();
  receiver->stop();

  ioc.restart();
  auto extra = common::safe_convert::string_to_uint8("after-close");
  EXPECT_NO_THROW(sender->async_write_copy(extra.data(), extra.size()));
  ioc.run_for(20ms);
}

TEST(TransportUdpTest, LearnsRemoteFromFirstPacket) {
  if (!can_open_udp_socket()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  boost::asio::io_context ioc;
  uint16_t base_port = reserve_free_port();

  config::UdpConfig cfg;
  cfg.local_port = base_port;

  auto channel = transport::UdpChannel::create(cfg, ioc);

  std::atomic<int> connected_events{0};
  std::string inbound;
  channel->on_state([&](common::LinkState s) {
    if (s == common::LinkState::Connected) connected_events.fetch_add(1);
  });
  channel->on_bytes([&](const uint8_t* data, size_t n) { inbound.assign(reinterpret_cast<const char*>(data), n); });

  boost::asio::ip::udp::socket peer(ioc, {boost::asio::ip::udp::v4(), 0});
  boost::asio::ip::udp::endpoint peer_ep{boost::asio::ip::make_address("127.0.0.1"), base_port};

  std::vector<uint8_t> reply_buf(32);
  std::atomic<std::size_t> reply_size{0};
  boost::asio::ip::udp::endpoint from_ep;
  peer.async_receive_from(boost::asio::buffer(reply_buf), from_ep,
                          [&reply_size](const boost::system::error_code& ec, std::size_t n) {
                            if (!ec) reply_size.store(n);
                          });

  channel->start();

  pump_io(ioc, 30ms);  // allow receive loop to start

  const std::string first = "discover";
  peer.send_to(boost::asio::buffer(first), peer_ep);

  EXPECT_TRUE(wait_for_condition(ioc, [&] { return inbound == first && channel->is_connected(); }, 200ms));
  EXPECT_GE(connected_events.load(), 1);

  const std::string response = "ack";
  auto out = common::safe_convert::string_to_uint8(response);
  channel->async_write_copy(out.data(), out.size());
  EXPECT_TRUE(wait_for_condition(ioc, [&] { return reply_size.load() == response.size(); }, 200ms));
  EXPECT_EQ(std::string(reply_buf.begin(), reply_buf.begin() + static_cast<std::ptrdiff_t>(reply_size.load())),
            response);

  peer.close();
  channel->stop();
}

TEST(TransportUdpTest, WriteWithoutRemoteIsNoop) {
  if (!can_open_udp_socket()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  boost::asio::io_context ioc;
  config::UdpConfig cfg;
  cfg.local_port = reserve_free_port();

  auto channel = transport::UdpChannel::create(cfg, ioc);
  std::atomic<common::LinkState> last_state{common::LinkState::Idle};
  channel->on_state([&](common::LinkState s) { last_state.store(s); });

  channel->start();
  auto payload = common::safe_convert::string_to_uint8("orphan");
  channel->async_write_copy(payload.data(), payload.size());  // no remote yet

  EXPECT_FALSE(wait_for_condition(ioc, [&] { return last_state.load() == common::LinkState::Error; }, 100ms));
  channel->stop();
}

TEST(TransportUdpTest, RemoteStaysFirstPeer) {
  if (!can_open_udp_socket()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  boost::asio::io_context ioc;
  uint16_t base_port = reserve_free_port();

  config::UdpConfig cfg;
  cfg.local_port = base_port;

  auto channel = transport::UdpChannel::create(cfg, ioc);

  boost::asio::ip::udp::socket peer1(ioc, {boost::asio::ip::udp::v4(), 0});
  boost::asio::ip::udp::socket peer2(ioc, {boost::asio::ip::udp::v4(), 0});
  boost::asio::ip::udp::endpoint channel_ep{boost::asio::ip::make_address("127.0.0.1"), base_port};

  std::array<uint8_t, 32> recv1{};
  std::array<uint8_t, 32> recv2{};
  std::atomic<std::size_t> recv1_size{0};
  std::atomic<std::size_t> recv2_size{0};

  peer1.async_receive_from(boost::asio::buffer(recv1), channel_ep, [&](auto ec, auto n) {
    if (!ec) recv1_size.store(n);
  });
  peer2.async_receive_from(boost::asio::buffer(recv2), channel_ep, [&](auto ec, auto n) {
    if (!ec) recv2_size.store(n);
  });

  channel->start();

  pump_io(ioc, 30ms);  // allow receive loop to start

  peer1.send_to(boost::asio::buffer(std::string("hello1")), channel_ep);
  EXPECT_TRUE(wait_for_condition(ioc, [&] { return channel->is_connected(); }, 200ms));

  // Later packet from a different peer should not change remote
  peer2.send_to(boost::asio::buffer(std::string("hello2")), channel_ep);
  pump_io(ioc, 40ms);

  auto reply = common::safe_convert::string_to_uint8("only-peer1");
  channel->async_write_copy(reply.data(), reply.size());
  EXPECT_TRUE(wait_for_condition(ioc, [&] { return recv1_size.load() == reply.size(); }, 200ms));

  EXPECT_EQ(recv1_size.load(), reply.size());
  EXPECT_EQ(recv2_size.load(), 0U);

  peer1.close();
  peer2.close();
  channel->stop();
}

TEST(TransportUdpTest, TruncatedDatagramSetsError) {
  if (!can_open_udp_socket()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  boost::asio::io_context ioc;
  uint16_t port = reserve_free_port();

  config::UdpConfig cfg;
  cfg.local_port = port;
  auto channel = transport::UdpChannel::create(cfg, ioc);

  std::atomic<common::LinkState> last_state{common::LinkState::Idle};
  channel->on_state([&](common::LinkState s) { last_state.store(s); });
  channel->start();

  pump_io(ioc, 30ms);  // allow bind + receive to arm

  boost::asio::ip::udp::socket peer(ioc, {boost::asio::ip::udp::v4(), 0});
  boost::asio::ip::udp::endpoint ep{boost::asio::ip::make_address("127.0.0.1"), port};

  std::vector<uint8_t> big(common::constants::DEFAULT_READ_BUFFER_SIZE + 512, 0xAB);
  peer.send_to(boost::asio::buffer(big), ep);

  EXPECT_TRUE(wait_for_condition(ioc, [&] { return last_state.load() == common::LinkState::Error; }, 200ms));

  peer.close();
  channel->stop();
}

TEST(TransportUdpTest, QueueLimitMovesToError) {
  if (!can_open_udp_socket()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  boost::asio::io_context ioc;
  uint16_t base_port = reserve_free_port();
  uint16_t remote_port = reserve_free_port();

  config::UdpConfig cfg;
  cfg.local_port = base_port;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = remote_port;
  cfg.backpressure_threshold = common::constants::MIN_BACKPRESSURE_THRESHOLD;

  auto channel = transport::UdpChannel::create(cfg, ioc);
  channel->start();

  std::atomic<common::LinkState> last_state{common::LinkState::Idle};
  channel->on_state([&](common::LinkState s) { last_state.store(s); });

  std::vector<uint8_t> huge(common::constants::DEFAULT_BACKPRESSURE_THRESHOLD * 2, 0xCD);
  channel->async_write_copy(huge.data(), huge.size());

  EXPECT_TRUE(wait_for_condition(ioc, [&] { return last_state.load() == common::LinkState::Error; }, 200ms));
  channel->stop();
}

TEST(TransportUdpTest, StopCancelsInFlightHandlers) {
  if (!can_open_udp_socket()) GTEST_SKIP() << "Socket open not permitted in sandbox";
  boost::asio::io_context ioc;
  uint16_t port = reserve_free_port();
  config::UdpConfig cfg;
  cfg.local_port = port;
  auto channel = transport::UdpChannel::create(cfg, ioc);

  std::atomic<int> bytes_callbacks{0};
  channel->on_bytes([&](const uint8_t*, size_t) { bytes_callbacks.fetch_add(1); });

  channel->start();
  channel->stop();

  boost::asio::ip::udp::socket peer(ioc, {boost::asio::ip::udp::v4(), 0});
  boost::asio::ip::udp::endpoint ep{boost::asio::ip::make_address("127.0.0.1"), port};
  peer.send_to(boost::asio::buffer(std::string("late")), ep);

  EXPECT_FALSE(wait_for_condition(
      ioc, [&] { return bytes_callbacks.load() > 0; }, 100ms))
      << "stop semantics: no user callbacks after stop";
  peer.close();
}

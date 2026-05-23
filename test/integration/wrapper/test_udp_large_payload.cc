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
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "test_utils.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/memory/safe_span.hpp"
#include "unilink/transport/udp/udp.hpp"
#include "unilink/wrapper/udp/udp.hpp"
#include "unilink/wrapper/udp/udp_server.hpp"

using namespace unilink;

namespace {

namespace net = boost::asio;
using udp = net::ip::udp;

std::string make_payload(size_t size) {
  std::string payload(size, '\0');
  for (size_t i = 0; i < size; ++i) {
    payload[i] = static_cast<char>((i * 31u + 17u) % 251u + 1u);
  }
  return payload;
}

uint16_t allocate_udp_port() {
  net::io_context ioc;
  udp::socket socket(ioc);
  socket.open(udp::v4());
  socket.bind(udp::endpoint(net::ip::make_address("127.0.0.1"), 0));
  const auto port = socket.local_endpoint().port();
  socket.close();
  return port;
}

bool supports_plain_udp_loopback_payload(size_t payload_size) {
  try {
    const auto payload = make_payload(payload_size);
    net::io_context ioc;
    udp::socket receiver(ioc, udp::endpoint(udp::v4(), 0));
    udp::socket sender(ioc, udp::endpoint(udp::v4(), 0));
    receiver.non_blocking(true);

    const auto receiver_endpoint = udp::endpoint(net::ip::make_address("127.0.0.1"), receiver.local_endpoint().port());
    const auto sent = sender.send_to(net::buffer(payload), receiver_endpoint);
    if (sent != payload.size()) {
      return false;
    }

    std::vector<uint8_t> buffer(payload.size() + 1);
    udp::endpoint sender_endpoint;
    return test::TestUtils::waitForCondition(
        [&] {
          boost::system::error_code ec;
          const auto bytes = receiver.receive_from(net::buffer(buffer), sender_endpoint, 0, ec);
          if (ec || bytes != payload.size()) {
            return false;
          }
          return std::string(reinterpret_cast<const char*>(buffer.data()), bytes) == payload;
        },
        500);
  } catch (...) {
    return false;
  }
}

config::UdpConfig make_server_config(uint16_t port, base::constants::BackpressureStrategy strategy) {
  config::UdpConfig cfg;
  cfg.bind_address = "127.0.0.1";
  cfg.local_port = port;
  cfg.backpressure_strategy = strategy;
  cfg.backpressure_threshold = 64 * 1024;
  return cfg;
}

config::UdpConfig make_client_config(uint16_t server_port, base::constants::BackpressureStrategy strategy) {
  config::UdpConfig cfg;
  cfg.bind_address = "127.0.0.1";
  cfg.local_port = 0;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = server_port;
  cfg.backpressure_strategy = strategy;
  cfg.backpressure_threshold = 64 * 1024;
  return cfg;
}

void expect_wrapper_echo(size_t payload_size, base::constants::BackpressureStrategy strategy) {
  if (!supports_plain_udp_loopback_payload(payload_size)) {
    GTEST_SKIP() << "plain UDP loopback did not deliver a " << payload_size
                 << "-byte datagram in this environment";
  }

  const auto payload = make_payload(payload_size);
  const auto port = allocate_udp_port();

  auto server = std::make_shared<wrapper::UdpServer>(make_server_config(port, strategy));
  std::mutex server_mutex;
  ClientId server_client_id{0};
  std::string server_payload;
  std::atomic<bool> server_received{false};

  server->on_data([&](const wrapper::MessageContext& ctx) {
    std::lock_guard<std::mutex> lock(server_mutex);
    server_client_id = ctx.client_id();
    server_payload = ctx.data_as_string();
    server_received.store(true);
  });

  auto server_started = server->start();
  ASSERT_TRUE(server_started.get());

  std::mutex received_mutex;
  std::string received;
  std::atomic<bool> echo_received{false};

  auto client = std::make_shared<wrapper::UdpClient>(make_client_config(port, strategy));
  client->on_data([&](const wrapper::MessageContext& ctx) {
    std::lock_guard<std::mutex> lock(received_mutex);
    received = ctx.data_as_string();
    echo_received.store(true);
  });

  auto client_started = client->start();
  ASSERT_TRUE(client_started.get());

  ASSERT_TRUE(client->try_send(payload));
  const bool server_saw_payload =
      test::TestUtils::waitForCondition([&] { return server_received.load(); }, 3000);
  if (!server_saw_payload) {
    client->stop();
    server->stop();
    ASSERT_TRUE(server_saw_payload);
  }

  ClientId client_id{0};
  std::string echo_payload;
  {
    std::lock_guard<std::mutex> lock(server_mutex);
    client_id = server_client_id;
    echo_payload = server_payload;
  }
  EXPECT_EQ(echo_payload, payload);
  const bool echo_started = server->try_send_to(client_id, echo_payload);
  if (!echo_started) {
    client->stop();
    server->stop();
    ASSERT_TRUE(echo_started);
  }

  const bool client_saw_echo = test::TestUtils::waitForCondition([&] { return echo_received.load(); }, 3000);
  if (!client_saw_echo) {
    client->stop();
    server->stop();
    ASSERT_TRUE(client_saw_echo);
  }

  {
    std::lock_guard<std::mutex> lock(received_mutex);
    EXPECT_EQ(received, payload);
  }

  const auto client_stats = client->stats();
  EXPECT_EQ(client_stats.failed_sends, 0u);
  EXPECT_EQ(client_stats.dropped_messages, 0u);
  EXPECT_EQ(client_stats.dropped_bytes, 0u);
  EXPECT_GE(client_stats.messages_accepted, 1u);

  const auto server_stats = server->stats();
  EXPECT_EQ(server_stats.failed_sends, 0u);
  EXPECT_EQ(server_stats.dropped_messages, 0u);
  EXPECT_EQ(server_stats.dropped_bytes, 0u);
  EXPECT_GE(server_stats.messages_received, 1u);
  EXPECT_GE(server_stats.bytes_received, payload.size());

  client->stop();
  server->stop();
}

}  // namespace

TEST(UdpLargePayloadIntegrationTest, WrapperReliableEchoes1024BytePayload) {
  expect_wrapper_echo(1024, base::constants::BackpressureStrategy::Reliable);
}

TEST(UdpLargePayloadIntegrationTest, WrapperReliableEchoes4096BytePayload) {
  expect_wrapper_echo(4096, base::constants::BackpressureStrategy::Reliable);
}

TEST(UdpLargePayloadIntegrationTest, WrapperReliableEchoes16384BytePayload) {
  expect_wrapper_echo(16384, base::constants::BackpressureStrategy::Reliable);
}

TEST(UdpLargePayloadIntegrationTest, WrapperBestEffortEchoes4096BytePayload) {
  expect_wrapper_echo(4096, base::constants::BackpressureStrategy::BestEffort);
}

TEST(UdpLargePayloadIntegrationTest, TransportEchoes4096BytePayload) {
  if (!supports_plain_udp_loopback_payload(4096)) {
    GTEST_SKIP() << "plain UDP loopback did not deliver a 4096-byte datagram in this environment";
  }

  const auto payload = make_payload(4096);
  const auto receiver_port = allocate_udp_port();

  auto receiver =
      transport::UdpChannel::create(make_server_config(receiver_port, base::constants::BackpressureStrategy::Reliable));
  auto sender =
      transport::UdpChannel::create(make_client_config(receiver_port, base::constants::BackpressureStrategy::Reliable));

  std::atomic<bool> receiver_ready{false};
  std::atomic<bool> sender_ready{false};
  std::atomic<bool> receiver_saw_payload{false};
  std::atomic<bool> received_echo{false};
  std::mutex received_mutex;
  std::string received;
  std::mutex receiver_mutex;
  std::string receiver_payload;
  udp::endpoint receiver_reply_endpoint;

  receiver->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Listening || state == base::LinkState::Connected) {
      receiver_ready.store(true);
    }
  });
  sender->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Connected) {
      sender_ready.store(true);
    }
  });

  receiver->on_bytes_from([&](memory::ConstByteSpan data, const udp::endpoint& endpoint) {
    std::lock_guard<std::mutex> lock(receiver_mutex);
    receiver_payload.assign(reinterpret_cast<const char*>(data.data()), data.size());
    receiver_reply_endpoint = endpoint;
    receiver_saw_payload.store(true);
  });
  sender->on_bytes([&](memory::ConstByteSpan data) {
    std::lock_guard<std::mutex> lock(received_mutex);
    received.assign(reinterpret_cast<const char*>(data.data()), data.size());
    received_echo.store(true);
  });

  receiver->start();
  sender->start();

  ASSERT_TRUE(test::TestUtils::waitForCondition([&] { return receiver_ready.load() && sender_ready.load(); }, 3000));
  ASSERT_TRUE(sender->async_write_copy(
      memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(payload.data()), payload.size())));
  const bool receiver_got_payload =
      test::TestUtils::waitForCondition([&] { return receiver_saw_payload.load(); }, 3000);
  if (!receiver_got_payload) {
    sender->stop();
    receiver->stop();
    ASSERT_TRUE(receiver_got_payload);
  }

  std::string echo_payload;
  udp::endpoint echo_endpoint;
  {
    std::lock_guard<std::mutex> lock(receiver_mutex);
    echo_payload = receiver_payload;
    echo_endpoint = receiver_reply_endpoint;
  }
  EXPECT_EQ(echo_payload, payload);
  ASSERT_TRUE(receiver->async_write_to(
      memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(echo_payload.data()), echo_payload.size()), echo_endpoint));

  const bool sender_got_echo = test::TestUtils::waitForCondition([&] { return received_echo.load(); }, 3000);
  if (!sender_got_echo) {
    sender->stop();
    receiver->stop();
    ASSERT_TRUE(sender_got_echo);
  }

  {
    std::lock_guard<std::mutex> lock(received_mutex);
    EXPECT_EQ(received, payload);
  }

  sender->stop();
  receiver->stop();
}

TEST(UdpLargePayloadIntegrationTest, TransportReceivesExternal4096BytePayload) {
  if (!supports_plain_udp_loopback_payload(4096)) {
    GTEST_SKIP() << "plain UDP loopback did not deliver a 4096-byte datagram in this environment";
  }

  const auto payload = make_payload(4096);
  const auto receiver_port = allocate_udp_port();
  auto receiver =
      transport::UdpChannel::create(make_server_config(receiver_port, base::constants::BackpressureStrategy::Reliable));

  std::atomic<bool> receiver_ready{false};
  std::atomic<bool> received_payload{false};
  std::mutex received_mutex;
  std::string received;

  receiver->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Listening || state == base::LinkState::Connected) {
      receiver_ready.store(true);
    }
  });
  receiver->on_bytes([&](memory::ConstByteSpan data) {
    std::lock_guard<std::mutex> lock(received_mutex);
    received.assign(reinterpret_cast<const char*>(data.data()), data.size());
    received_payload.store(true);
  });

  receiver->start();
  ASSERT_TRUE(test::TestUtils::waitForCondition([&] { return receiver_ready.load(); }, 3000));

  net::io_context ioc;
  udp::socket socket(ioc, udp::endpoint(udp::v4(), 0));
  socket.send_to(net::buffer(payload), udp::endpoint(net::ip::make_address("127.0.0.1"), receiver_port));

  const bool receiver_got_payload = test::TestUtils::waitForCondition([&] { return received_payload.load(); }, 3000);
  receiver->stop();
  ASSERT_TRUE(receiver_got_payload);

  std::lock_guard<std::mutex> lock(received_mutex);
  EXPECT_EQ(received, payload);
}

TEST(UdpLargePayloadIntegrationTest, TransportSends4096BytePayloadToExternalSocket) {
  if (!supports_plain_udp_loopback_payload(4096)) {
    GTEST_SKIP() << "plain UDP loopback did not deliver a 4096-byte datagram in this environment";
  }

  const auto payload = make_payload(4096);

  net::io_context ioc;
  udp::socket socket(ioc, udp::endpoint(udp::v4(), 0));
  socket.non_blocking(true);
  const auto external_port = socket.local_endpoint().port();

  auto sender =
      transport::UdpChannel::create(make_client_config(external_port, base::constants::BackpressureStrategy::Reliable));

  std::atomic<bool> sender_ready{false};
  sender->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Connected) {
      sender_ready.store(true);
    }
  });

  sender->start();
  ASSERT_TRUE(test::TestUtils::waitForCondition([&] { return sender_ready.load(); }, 3000));
  ASSERT_TRUE(sender->async_write_copy(
      memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(payload.data()), payload.size())));

  std::vector<uint8_t> buffer(payload.size());
  udp::endpoint sender_endpoint;
  bool received_payload = test::TestUtils::waitForCondition(
      [&] {
        boost::system::error_code ec;
        const auto bytes = socket.receive_from(net::buffer(buffer), sender_endpoint, 0, ec);
        return !ec && bytes == payload.size();
      },
      3000);

  sender->stop();
  ASSERT_TRUE(received_payload);
  EXPECT_EQ(std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size()), payload);
}

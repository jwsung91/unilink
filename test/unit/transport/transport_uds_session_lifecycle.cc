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
#include <functional>
#include <vector>

#include "../../mocks/mock_uds_socket.hpp"
#include "test_utils.hpp"
#include "unilink/transport/uds/boost_uds_socket.hpp"
#include "unilink/transport/uds/uds_server_session.hpp"

using namespace unilink;
using namespace transport;
using namespace testing;
using namespace std::chrono_literals;

namespace unilink {
namespace test {

class UdsServerSessionLifecycleTest : public Test {
 protected:
  boost::asio::io_context ioc;
};

TEST_F(UdsServerSessionLifecycleTest, RedundantStop) {
  auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
  EXPECT_CALL(*mock_socket, close(_)).Times(1);

  auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 0);
  session->start();
  session->stop();
  session->stop();  // Should return early

  ioc.run();
}

TEST_F(UdsServerSessionLifecycleTest, BackpressureLimitEnforced) {
  auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
  auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 100, 0);

  std::vector<uint8_t> large_data(base::constants::DEFAULT_BACKPRESSURE_THRESHOLD + 1, 'A');
  EXPECT_FALSE(session->async_write_copy(memory::ConstByteSpan(large_data.data(), large_data.size())));

  // We can't easily check internal state but we can verify it doesn't crash
  // and exercises the bp_limit check.
  session->stop();
  ioc.run();
}

TEST_F(UdsServerSessionLifecycleTest, IdleTimeoutExpiration) {
  auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
  EXPECT_CALL(*mock_socket, async_read_some(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_socket, close(_)).Times(1);

  std::atomic<bool> closed{false};
  auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 10);  // 10ms timeout
  session->on_close([&]() { closed = true; });
  session->start();

  // Manually run io_context in a loop to allow timer and handlers to execute
  auto start = std::chrono::steady_clock::now();
  while (!closed && std::chrono::steady_clock::now() - start < 1s) {
    ioc.run_one_for(10ms);
  }

  EXPECT_TRUE(closed.load());
}

TEST_F(UdsServerSessionLifecycleTest, ReadErrorClosesSession) {
  auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
  EXPECT_CALL(*mock_socket, async_read_some(_, _))
      .WillOnce(Invoke([](const boost::asio::mutable_buffer&,
                          std::function<void(const boost::system::error_code&, std::size_t)> handler) {
        handler(make_error_code(boost::asio::error::connection_reset), 0);
      }));
  EXPECT_CALL(*mock_socket, close(_)).Times(1);

  std::atomic<bool> closed{false};
  auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 0);
  session->on_close([&]() { closed = true; });
  session->start();

  ioc.run();

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

TEST_F(UdsServerSessionLifecycleTest, WriteErrorClosesSession) {
  auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
  EXPECT_CALL(*mock_socket, async_read_some(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_socket, async_write(_, _))
      .WillOnce(Invoke([](const boost::asio::const_buffer& buffer,
                          std::function<void(const boost::system::error_code&, std::size_t)> handler) {
        handler(make_error_code(boost::asio::error::broken_pipe), buffer.size());
      }));
  EXPECT_CALL(*mock_socket, close(_)).Times(1);

  std::atomic<bool> closed{false};
  auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 0);
  session->on_close([&]() { closed = true; });
  session->start();

  std::vector<uint8_t> payload(32, 'x');
  EXPECT_TRUE(session->async_write_move(std::move(payload)));

  ioc.run();

  EXPECT_TRUE(closed.load());
  EXPECT_FALSE(session->alive());
}

TEST_F(UdsServerSessionLifecycleTest, SharedWritePendingFlushesAfterBackpressureRelief) {
  auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
  std::function<void(const boost::system::error_code&, std::size_t)> first_write_handler;
  int write_calls = 0;

  EXPECT_CALL(*mock_socket, async_read_some(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_socket, async_write(_, _))
      .WillRepeatedly(Invoke([&](const boost::asio::const_buffer& buffer,
                                 std::function<void(const boost::system::error_code&, std::size_t)> handler) {
        ++write_calls;
        if (write_calls == 1) {
          first_write_handler = std::move(handler);
          return;
        }
        handler({}, buffer.size());
      }));
  EXPECT_CALL(*mock_socket, close(_)).Times(1);

  auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 0);
  std::vector<size_t> backpressure_events;
  session->on_backpressure([&](size_t queued) { backpressure_events.push_back(queued); });
  session->start();

  auto payload = std::make_shared<const std::vector<uint8_t>>(2048, 'p');
  EXPECT_TRUE(session->async_write_shared(payload));
  ioc.run_for(10ms);
  ASSERT_TRUE(first_write_handler);
  EXPECT_TRUE(session->is_backpressure_active());

  EXPECT_TRUE(session->async_write_shared(std::make_shared<const std::vector<uint8_t>>(64, 'q')));
  ioc.run_for(10ms);
  EXPECT_EQ(write_calls, 1);

  first_write_handler({}, payload->size());
  ioc.restart();
  ioc.run_for(20ms);

  EXPECT_GE(write_calls, 2);
  EXPECT_FALSE(backpressure_events.empty());

  session->stop();
  ioc.restart();
  ioc.run();
}

TEST_F(UdsServerSessionLifecycleTest, SharedWriteRejectsNullAndClosedSession) {
  auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
  EXPECT_CALL(*mock_socket, close(_)).Times(1);

  auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 0);
  EXPECT_FALSE(session->async_write_shared(nullptr));

  session->start();
  session->stop();
  ioc.run();

  EXPECT_FALSE(session->async_write_shared(std::make_shared<const std::vector<uint8_t>>(8, 'z')));
}

}  // namespace test
}  // namespace unilink

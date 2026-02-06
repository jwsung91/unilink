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
#include <future>
#include <memory>
#include <thread>

#include "unilink/transport/tcp_server/tcp_server_session.hpp"

using namespace unilink;
using namespace unilink::transport;

namespace net = boost::asio;
using tcp = net::ip::tcp;

class TcpCancelTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TcpCancelTest, CancelTriggersErrorHandling) {
  net::io_context ioc;
  auto work_guard = net::make_work_guard(ioc);

  // Setup Acceptor on ephemeral port
  tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
  uint16_t port = acceptor.local_endpoint().port();

  tcp::socket server_socket(ioc);
  tcp::socket client_socket(ioc);

  std::promise<void> accept_promise;
  auto accept_future = accept_promise.get_future();

  acceptor.async_accept(server_socket, [&](boost::system::error_code ec) {
    if (!ec) {
      accept_promise.set_value();
    } else {
      FAIL() << "Accept failed: " << ec.message();
    }
  });

  client_socket.async_connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), port),
                              [&](boost::system::error_code ec) {
                                if (ec) {
                                  FAIL() << "Connect failed: " << ec.message();
                                }
                              });

  std::thread ioc_thread([&ioc] { ioc.run(); });

  // Wait for connection
  ASSERT_EQ(accept_future.wait_for(std::chrono::seconds(5)), std::future_status::ready);

  // Create session with the connected server socket
  auto session = std::make_shared<TcpServerSession>(ioc, std::move(server_socket));

  std::promise<void> close_promise;
  auto close_future = close_promise.get_future();

  session->on_close([&]() { close_promise.set_value(); });

  session->start();

  // Allow read loop to start and wait for data
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Trigger cancel() - this should cause async_read_some to complete with
  // operation_aborted and trigger the error handling path which calls
  // do_close() -> on_close callback.
  session->cancel();

  // Verify on_close is called
  ASSERT_EQ(close_future.wait_for(std::chrono::seconds(2)), std::future_status::ready);

  // Clean up
  ioc.stop();
  if (ioc_thread.joinable()) {
    ioc_thread.join();
  }
}

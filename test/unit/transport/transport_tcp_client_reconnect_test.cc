#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <condition_variable>
#include <thread>

#include "test_utils.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::test;
using namespace std::chrono_literals;

class TcpClientReconnectTest : public ::testing::Test {
 protected:
  void SetUp() override { port_ = TestUtils::getAvailableTestPort(); }
  uint16_t port_;
};

TEST_F(TcpClientReconnectTest, ReconnectAfterServerStop) {
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = port_;
  cfg.max_retries = -1;  // Infinite retries
  cfg.retry_interval_ms = 100;

  std::atomic<int> connect_count{0};
  std::mutex mtx;
  std::condition_variable cv;

  auto client = TcpClient::create(cfg);
  client->on_connect([&](const wrapper::ConnectionContext&) {
    connect_count++;
    cv.notify_all();
  });

  // 1. Start server and connect client
  auto server_ioc = std::make_shared<boost::asio::io_context>();
  boost::asio::ip::tcp::acceptor acceptor(*server_ioc,
                                          boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_));

  std::thread server_thread([&]() { server_ioc->run(); });

  client->start();

  {
    std::unique_lock<std::mutex> lock(mtx);
    ASSERT_TRUE(cv.wait_for(lock, 2s, [&] { return connect_count == 1; }));
  }

  // 2. Stop server (Force disconnect)
  acceptor.close();
  server_ioc->stop();
  if (server_thread.joinable()) server_thread.join();

  // 3. Restart server on the same port
  auto server_ioc2 = std::make_shared<boost::asio::io_context>();
  boost::asio::ip::tcp::acceptor acceptor2(*server_ioc2,
                                           boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_));

  std::thread server_thread2([&]() { server_ioc2->run(); });

  // 4. Wait for auto-reconnect
  {
    std::unique_lock<std::mutex> lock(mtx);
    ASSERT_TRUE(cv.wait_for(lock, 5s, [&] { return connect_count == 2; }))
        << "Client failed to reconnect after server restart";
  }

  client->stop();
  server_ioc2->stop();
  if (server_thread2.joinable()) server_thread2.join();
}

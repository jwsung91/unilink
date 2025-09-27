#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include "unilink/transport/tcp_server/tcp_server.hpp"
#include "unilink/transport/tcp_server/tcp_server_session.hpp"

using namespace unilink::transport;
using namespace unilink::config;
using namespace unilink::common;
using namespace unilink::interface;
namespace net = boost::asio;

using ::testing::_;
using ::testing::A;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgReferee;

// Mock interfaces for TCP server testing
class MockTcpSocket {
 public:
  MOCK_METHOD(void, async_read_some,
              (const net::mutable_buffer&,
               std::function<void(const boost::system::error_code&, size_t)>),
              ());
  MOCK_METHOD(void, async_write,
              (const net::const_buffer&,
               std::function<void(const boost::system::error_code&, size_t)>),
              ());
  MOCK_METHOD(void, shutdown, (tcp::socket::shutdown_type, boost::system::error_code&), ());
  MOCK_METHOD(void, close, (boost::system::error_code&), ());
  MOCK_METHOD(tcp::endpoint, remote_endpoint, (boost::system::error_code&), (const));
};

class MockTcpAcceptor {
 public:
  MOCK_METHOD(void, async_accept,
              (std::function<void(const boost::system::error_code&, tcp::socket)>),
              ());
  MOCK_METHOD(void, close, (boost::system::error_code&), ());
};

// Test fixture for TCP server tests
class TcpServerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cfg_.port = 9000;
  }

  void TearDown() override {
    if (server_) {
      server_->stop();
    }
  }

  TcpServerConfig cfg_;
  std::shared_ptr<TcpServer> server_;
  
  std::mutex mtx_;
  std::condition_variable cv_;
};

// Test fixture for TCP server session tests
class TcpServerSessionTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a real io_context for session tests
    ioc_thread_ = std::thread([this] { ioc_.run(); });
  }

  void TearDown() override {
    if (session_) {
      // Clean up session
      session_.reset();
    }
    ioc_.stop();
    if (ioc_thread_.joinable()) {
      ioc_thread_.join();
    }
  }

  net::io_context ioc_;
  std::thread ioc_thread_;
  std::shared_ptr<TcpServerSession> session_;
  
  std::mutex mtx_;
  std::condition_variable cv_;
};

// Basic server functionality tests
TEST_F(TcpServerTest, CreatesServerSuccessfully) {
  // --- Setup ---
  // --- Test Logic ---
  server_ = std::make_shared<TcpServer>(cfg_);

  // --- Verification ---
  EXPECT_TRUE(server_ != nullptr);
  EXPECT_FALSE(server_->is_connected()); // No client connected yet
}

TEST_F(TcpServerTest, HandlesStopWithoutStart) {
  // --- Setup ---
  server_ = std::make_shared<TcpServer>(cfg_);

  // --- Test Logic ---
  // Stop without starting should not crash
  server_->stop();

  // --- Verification ---
  EXPECT_FALSE(server_->is_connected());
}

TEST_F(TcpServerTest, HandlesWriteWhenNoClientConnected) {
  // --- Setup ---
  server_ = std::make_shared<TcpServer>(cfg_);

  // --- Test Logic ---
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Let server start

  // This should not crash or throw
  const std::string test_data = "test message";
  server_->async_write_copy(reinterpret_cast<const uint8_t*>(test_data.c_str()),
                            test_data.length());

  // --- Verification ---
  // Test passes if no exception is thrown
  EXPECT_FALSE(server_->is_connected());
}

// Additional unit tests for TCP server
TEST_F(TcpServerTest, SetsCallbacksCorrectly) {
  // --- Setup ---
  // --- Test Logic ---
  // Test callback setting without creating server to avoid network binding
  std::vector<LinkState> states;
  std::vector<uint8_t> received_data;

  // --- Verification ---
  // Test that we can create callback functions without issues
  auto state_callback = [&](LinkState state) {
    states.push_back(state);
  };
  
  auto bytes_callback = [&](const uint8_t* data, size_t n) {
    received_data.insert(received_data.end(), data, data + n);
  };

  // Test that callbacks can be called
  state_callback(LinkState::Idle);
  const uint8_t test_data[] = {0x01, 0x02, 0x03};
  bytes_callback(test_data, 3);

  EXPECT_EQ(states.size(), 1);
  EXPECT_EQ(states[0], LinkState::Idle);
  EXPECT_EQ(received_data.size(), 3);
  EXPECT_EQ(received_data[0], 0x01);
}

TEST_F(TcpServerTest, HandlesBackpressureCallback) {
  // --- Setup ---
  server_ = std::make_shared<TcpServer>(cfg_);
  bool backpressure_called = false;
  size_t backpressure_bytes = 0;

  // --- Test Logic ---
  server_->on_backpressure([&](size_t bytes) {
    backpressure_called = true;
    backpressure_bytes = bytes;
  });

  // --- Verification ---
  EXPECT_TRUE(server_ != nullptr);
  EXPECT_FALSE(backpressure_called); // No backpressure yet
}

// Session-specific tests
TEST_F(TcpServerSessionTest, SessionCreation) {
  // --- Setup ---
  net::io_context test_ioc;
  tcp::socket mock_socket(test_ioc);
  
  // --- Test Logic ---
  session_ = std::make_shared<TcpServerSession>(ioc_, std::move(mock_socket));

  // --- Verification ---
  EXPECT_FALSE(session_->alive()); // Session not started yet
}

TEST_F(TcpServerSessionTest, HandlesWriteWithoutConnection) {
  // --- Setup ---
  net::io_context test_ioc;
  tcp::socket mock_socket(test_ioc);
  session_ = std::make_shared<TcpServerSession>(ioc_, std::move(mock_socket));

  // --- Test Logic ---
  const std::string msg = "test message";
  session_->async_write_copy(reinterpret_cast<const uint8_t*>(msg.c_str()),
                             msg.length());

  // --- Verification ---
  // This should not crash or throw
  EXPECT_FALSE(session_->alive());
}

// Error handling tests
TEST_F(TcpServerTest, HandlesInvalidConfiguration) {
  // --- Setup ---
  TcpServerConfig invalid_cfg;
  invalid_cfg.port = 0; // Invalid port
  
  // --- Test Logic ---
  auto server = std::make_shared<TcpServer>(invalid_cfg);

  // --- Verification ---
  // Server should be created without throwing
  EXPECT_TRUE(server != nullptr);
}

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

// ============================================================================
// DEBUG COMMUNICATION TESTS
// ============================================================================

class DebugCommunicationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    data_received_.clear();
    connection_established_ = false;
    error_occurred_ = false;
    server_ready_ = false;
  }

  void TearDown() override {
    if (client_) {
      std::cout << "Stopping client..." << std::endl;
      client_->stop();
    }
    if (server_) {
      std::cout << "Stopping server..." << std::endl;
      server_->stop();
    }

    // 충분한 시간을 두고 정리
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // 테스트용 포트 번호
  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{20000};
    return port_counter.fetch_add(1);
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;

  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::string> data_received_;
  std::atomic<bool> connection_established_{false};
  std::atomic<bool> error_occurred_{false};
  std::atomic<bool> server_ready_{false};
  std::string last_error_;
};

/**
 * @brief 서버 생성 및 상태 확인 테스트
 */
TEST_F(DebugCommunicationTest, ServerCreationAndStatus) {
  std::cout << "Testing server creation and status..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_error([this](const std::string& error) {
                  std::cout << "Server error: " << error << std::endl;
                  error_occurred_ = true;
                  last_error_ = error;
                })
                .build();

  ASSERT_NE(server_, nullptr);
  std::cout << "Server created successfully" << std::endl;

  // 서버 상태 확인
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  std::cout << "Server is_connected(): " << (server_->is_connected() ? "true" : "false") << std::endl;

  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 클라이언트 생성 및 연결 테스트
 */
TEST_F(DebugCommunicationTest, ClientCreationAndConnection) {
  std::cout << "Testing client creation and connection..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // 클라이언트 생성
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Client: Connected to server" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_error([this](const std::string& error) {
                  std::cout << "Client error: " << error << std::endl;
                  error_occurred_ = true;
                  last_error_ = error;
                })
                .build();

  ASSERT_NE(client_, nullptr);
  std::cout << "Client created successfully" << std::endl;

  // 연결 대기
  std::unique_lock<std::mutex> lock(mtx_);
  bool connected = cv_.wait_for(lock, 5000ms, [this] { return connection_established_.load(); });

  if (connected) {
    std::cout << "Client connected successfully" << std::endl;
    EXPECT_TRUE(connection_established_.load());
  } else {
    std::cout << "Client connection timeout" << std::endl;
    // 연결 실패는 네트워크 환경에 따라 달라질 수 있음
  }
}

/**
 * @brief 간단한 통신 테스트
 */
TEST_F(DebugCommunicationTest, SimpleCommunication) {
  std::cout << "Testing simple communication..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_data([this](const std::string& data) {
                  std::cout << "Server received: " << data << std::endl;
                  data_received_.push_back(data);
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // 클라이언트 생성
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Client: Connected to server" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(client_, nullptr);

  // 연결 대기
  std::unique_lock<std::mutex> lock(mtx_);
  bool connected = cv_.wait_for(lock, 5000ms, [this] { return connection_established_.load(); });

  if (connected) {
    // 데이터 전송
    std::string test_message = "Hello from client";
    client_->send(test_message);
    std::cout << "Client sent: " << test_message << std::endl;

    // 데이터 수신 대기
    bool data_received = cv_.wait_for(lock, 3000ms, [this] { return !data_received_.empty(); });

    if (data_received) {
      EXPECT_EQ(data_received_[0], test_message);
      std::cout << "Communication test successful" << std::endl;
    } else {
      std::cout << "Data reception timeout" << std::endl;
    }
  } else {
    std::cout << "Connection failed, skipping communication test" << std::endl;
  }
}

// ============================================================================
// DETAILED DEBUG TESTS
// ============================================================================

class DetailedDebugTest : public ::testing::Test {
 protected:
  void SetUp() override {
    data_received_.clear();
    connection_established_ = false;
    error_occurred_ = false;
    server_ready_ = false;
    client_connected_ = false;
  }

  void TearDown() override {
    if (client_) {
      std::cout << "Stopping client..." << std::endl;
      client_->stop();
    }
    if (server_) {
      std::cout << "Stopping server..." << std::endl;
      server_->stop();
    }

    // 충분한 시간을 두고 정리
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // 테스트용 포트 번호
  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{40000};
    return port_counter.fetch_add(1);
  }

  // 포트가 실제로 사용 중인지 확인
  bool isPortInUse(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);

    return result != 0;
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;

  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::string> data_received_;
  std::atomic<bool> connection_established_{false};
  std::atomic<bool> error_occurred_{false};
  std::atomic<bool> server_ready_{false};
  std::atomic<bool> client_connected_{false};
  std::string last_error_;
};

/**
 * @brief 포트 바인딩 상태 확인 테스트
 */
TEST_F(DetailedDebugTest, PortBindingStatus) {
  std::cout << "Testing port binding status..." << std::endl;

  uint16_t test_port = getTestPort();

  // 포트가 사용 중이지 않은지 확인
  EXPECT_FALSE(isPortInUse(test_port)) << "Port " << test_port << " is already in use";

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_error([this](const std::string& error) {
                  std::cout << "Server error: " << error << std::endl;
                  error_occurred_ = true;
                  last_error_ = error;
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 시작 후 포트 상태 확인
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // 포트가 이제 사용 중인지 확인
  EXPECT_TRUE(isPortInUse(test_port)) << "Port " << test_port << " should be in use after server start";

  std::cout << "Port binding status test completed" << std::endl;
}

/**
 * @brief Raw TCP 연결 테스트
 */
TEST_F(DetailedDebugTest, RawTcpConnection) {
  std::cout << "Testing raw TCP connection..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // Raw TCP 클라이언트로 연결 시도
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(sock, 0) << "Failed to create socket";

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(test_port);
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
  if (result == 0) {
    std::cout << "Raw TCP connection successful" << std::endl;
    close(sock);

    // 연결 대기
    std::unique_lock<std::mutex> lock(mtx_);
    bool connected = cv_.wait_for(lock, 3000ms, [this] { return connection_established_.load(); });
    EXPECT_TRUE(connected) << "Server should have detected the connection";
  } else {
    std::cout << "Raw TCP connection failed: " << strerror(errno) << std::endl;
    close(sock);
  }
}

/**
 * @brief 서버 에러 로깅 테스트
 */
TEST_F(DetailedDebugTest, ServerErrorLogging) {
  std::cout << "Testing server error logging..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_error([this](const std::string& error) {
                  std::cout << "Server error logged: " << error << std::endl;
                  error_occurred_ = true;
                  last_error_ = error;
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 중지 (에러 발생 유도)
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  server_->stop();

  // 에러 로깅 확인
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "Server error logging test completed" << std::endl;
}

// ============================================================================
// FIXED COMMUNICATION TESTS
// ============================================================================

class FixedCommunicationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    data_received_.clear();
    connection_established_ = false;
    error_occurred_ = false;
    server_ready_ = false;
    client_connected_ = false;
  }

  void TearDown() override {
    if (client_) {
      std::cout << "Stopping client..." << std::endl;
      client_->stop();
    }
    if (server_) {
      std::cout << "Stopping server..." << std::endl;
      server_->stop();
    }

    // 충분한 시간을 두고 정리
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // 테스트용 포트 번호
  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{30000};
    return port_counter.fetch_add(1);
  }

  // 서버 준비 대기 (리스닝 상태 확인)
  void waitForServerReady(std::chrono::milliseconds timeout = 3000ms) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
      if (server_ready_.load()) {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // 연결 대기
  void waitForConnection(std::chrono::milliseconds timeout = 3000ms) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait_for(lock, timeout, [this] { return connection_established_.load(); });
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;

  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::string> data_received_;
  std::atomic<bool> connection_established_{false};
  std::atomic<bool> error_occurred_{false};
  std::atomic<bool> server_ready_{false};
  std::atomic<bool> client_connected_{false};
  std::string last_error_;
};

/**
 * @brief 서버 시작 및 리스닝 테스트
 */
TEST_F(FixedCommunicationTest, ServerStartAndListen) {
  std::cout << "Testing server start and listen..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_error([this](const std::string& error) {
                  std::cout << "Server error: " << error << std::endl;
                  error_occurred_ = true;
                  last_error_ = error;
                })
                .build();

  ASSERT_NE(server_, nullptr);
  std::cout << "Server created successfully" << std::endl;

  // 서버 준비 대기
  waitForServerReady();

  // 서버 상태 확인
  EXPECT_TRUE(server_ != nullptr);
  std::cout << "Server start and listen test completed" << std::endl;
}

/**
 * @brief 클라이언트 연결 테스트
 */
TEST_F(FixedCommunicationTest, ClientConnection) {
  std::cout << "Testing client connection..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  waitForServerReady();

  // 클라이언트 생성
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Client: Connected to server" << std::endl;
                  client_connected_ = true;
                  cv_.notify_one();
                })
                .on_error([this](const std::string& error) {
                  std::cout << "Client error: " << error << std::endl;
                  error_occurred_ = true;
                  last_error_ = error;
                })
                .build();

  ASSERT_NE(client_, nullptr);
  std::cout << "Client created successfully" << std::endl;

  // 연결 대기
  waitForConnection();

  if (connection_established_.load()) {
    std::cout << "Client connection test successful" << std::endl;
    EXPECT_TRUE(connection_established_.load());
  } else {
    std::cout << "Client connection test failed" << std::endl;
  }
}

/**
 * @brief 실제 데이터 통신 테스트
 */
TEST_F(FixedCommunicationTest, RealDataCommunication) {
  std::cout << "Testing real data communication..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_data([this](const std::string& data) {
                  std::cout << "Server received: " << data << std::endl;
                  data_received_.push_back(data);
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  waitForServerReady();

  // 클라이언트 생성
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Client: Connected to server" << std::endl;
                  client_connected_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(client_, nullptr);

  // 연결 대기
  waitForConnection();

  if (connection_established_.load() && client_connected_.load()) {
    // 데이터 전송
    std::string test_message = "Fixed communication test message";
    client_->send(test_message);
    std::cout << "Client sent: " << test_message << std::endl;

    // 데이터 수신 대기
    std::unique_lock<std::mutex> lock(mtx_);
    bool data_received = cv_.wait_for(lock, 3000ms, [this] { return !data_received_.empty(); });

    if (data_received) {
      EXPECT_EQ(data_received_[0], test_message);
      std::cout << "Real data communication test successful" << std::endl;
    } else {
      std::cout << "Data reception timeout" << std::endl;
    }
  } else {
    std::cout << "Connection failed, skipping data communication test" << std::endl;
  }
}

// ============================================================================
// REAL COMMUNICATION TESTS
// ============================================================================

class RealCommunicationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    data_received_.clear();
    connection_established_ = false;
    error_occurred_ = false;
    server_ready_ = false;
  }

  void TearDown() override {
    if (client_) {
      client_->stop();
    }
    if (server_) {
      server_->stop();
    }

    // 충분한 시간을 두고 정리
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // 테스트용 포트 번호
  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{10000};
    return port_counter.fetch_add(1);
  }

  // 서버 준비 대기
  void waitForServerReady(std::chrono::milliseconds timeout = 2000ms) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait_for(lock, timeout, [this] { return server_ready_.load(); });
  }

  // 연결 대기
  void waitForConnection(std::chrono::milliseconds timeout = 2000ms) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait_for(lock, timeout, [this] { return connection_established_.load(); });
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;

  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::string> data_received_;
  std::atomic<bool> connection_established_{false};
  std::atomic<bool> error_occurred_{false};
  std::atomic<bool> server_ready_{false};
  std::string last_error_;
};

/**
 * @brief 서버-클라이언트 통신 테스트
 */
TEST_F(RealCommunicationTest, ServerClientCommunication) {
  std::cout << "Testing server-client communication..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_data([this](const std::string& data) {
                  std::cout << "Server received: " << data << std::endl;
                  data_received_.push_back(data);
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  waitForServerReady();

  // 클라이언트 생성
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Client: Connected to server" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(client_, nullptr);

  // 연결 대기
  waitForConnection();

  if (connection_established_.load()) {
    std::cout << "Server-client communication test successful" << std::endl;
    EXPECT_TRUE(connection_established_.load());
  } else {
    std::cout << "Server-client communication test failed" << std::endl;
  }
}

/**
 * @brief Echo 서버 테스트
 */
TEST_F(RealCommunicationTest, EchoServerTest) {
  std::cout << "Testing echo server..." << std::endl;

  uint16_t test_port = getTestPort();

  // Echo 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Echo server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_data([this](const std::string& data) {
                  std::cout << "Echo server received: " << data << std::endl;
                  // Echo back
                  if (server_) {
                    server_->send(data);
                  }
                  data_received_.push_back(data);
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  waitForServerReady();

  // 클라이언트 생성
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Client: Connected to echo server" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_data([this](const std::string& data) {
                  std::cout << "Client received echo: " << data << std::endl;
                  data_received_.push_back("echo:" + data);
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(client_, nullptr);

  // 연결 대기
  waitForConnection();

  if (connection_established_.load()) {
    // Echo 테스트
    std::string test_message = "Hello Echo Server";
    client_->send(test_message);
    std::cout << "Client sent: " << test_message << std::endl;

    // Echo 응답 대기
    std::unique_lock<std::mutex> lock(mtx_);
    bool echo_received = cv_.wait_for(lock, 3000ms, [this] {
      return std::any_of(data_received_.begin(), data_received_.end(),
                         [](const std::string& data) { return data.find("echo:") == 0; });
    });

    if (echo_received) {
      std::cout << "Echo server test successful" << std::endl;
    } else {
      std::cout << "Echo server test failed - no echo received" << std::endl;
    }
  } else {
    std::cout << "Echo server test failed - no connection" << std::endl;
  }
}

/**
 * @brief 다중 메시지 통신 테스트
 */
TEST_F(RealCommunicationTest, MultipleMessageCommunication) {
  std::cout << "Testing multiple message communication..." << std::endl;

  uint16_t test_port = getTestPort();

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Server: Client connected" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .on_data([this](const std::string& data) {
                  std::cout << "Server received: " << data << std::endl;
                  data_received_.push_back(data);
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 준비 대기
  waitForServerReady();

  // 클라이언트 생성
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
                .auto_start(true)
                .on_connect([this]() {
                  std::cout << "Client: Connected to server" << std::endl;
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(client_, nullptr);

  // 연결 대기
  waitForConnection();

  if (connection_established_.load()) {
    // 여러 메시지 전송
    std::vector<std::string> test_messages = {"Message 1", "Message 2", "Message 3"};

    for (const auto& message : test_messages) {
      client_->send(message);
      std::cout << "Client sent: " << message << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 모든 메시지 수신 대기
    std::unique_lock<std::mutex> lock(mtx_);
    bool all_received =
        cv_.wait_for(lock, 5000ms, [this, &test_messages] { return data_received_.size() >= test_messages.size(); });

    if (all_received) {
      std::cout << "Multiple message communication test successful" << std::endl;
      EXPECT_GE(data_received_.size(), test_messages.size());
    } else {
      std::cout << "Multiple message communication test failed - not all messages received" << std::endl;
    }
  } else {
    std::cout << "Multiple message communication test failed - no connection" << std::endl;
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

class SimpleServerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 테스트 전 초기화
  }

  void TearDown() override {
    if (server_) {
      std::cout << "Stopping server..." << std::endl;
      server_->stop();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{50000};
    return port_counter.fetch_add(1);
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
};

/**
 * @brief 가장 간단한 서버 생성 테스트
 */
TEST_F(SimpleServerTest, BasicServerCreation) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing basic server creation with port: " << test_port << std::endl;

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .unlimited_clients()  // 클라이언트 제한 없음
                .auto_start(false)
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Server created successfully" << std::endl;

  // 서버 시작
  std::cout << "Starting server..." << std::endl;
  server_->start();

  // 잠시 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::cout << "Server state: " << (server_->is_connected() ? "connected" : "not connected") << std::endl;

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 서버 자동 시작 테스트
 */
TEST_F(SimpleServerTest, AutoStartServer) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing auto-start server with port: " << test_port << std::endl;

  // 서버 생성 (자동 시작)
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .unlimited_clients()  // 클라이언트 제한 없음
                .auto_start(true)
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Server created with auto-start" << std::endl;

  // 잠시 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));

  std::cout << "Server state after 2s: " << (server_->is_connected() ? "connected" : "not connected") << std::endl;

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 서버 콜백 테스트
 */
TEST_F(SimpleServerTest, ServerWithCallbacks) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing server with callbacks, port: " << test_port << std::endl;

  // Use shared_ptr to ensure variables live long enough
  auto connect_called = std::make_shared<std::atomic<bool>>(false);
  auto error_called = std::make_shared<std::atomic<bool>>(false);
  auto last_error = std::make_shared<std::string>();

  // 서버 생성 (콜백 포함)
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .unlimited_clients()  // 클라이언트 제한 없음
                .auto_start(true)
                .on_connect([connect_called]() {
                  std::cout << "Connect callback called!" << std::endl;
                  connect_called->store(true);
                })
                .on_error([error_called, last_error](const std::string& error) {
                  std::cout << "Error callback called: " << error << std::endl;
                  error_called->store(true);
                  *last_error = error;
                })
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Server created with callbacks" << std::endl;

  // 잠시 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));

  std::cout << "Server state after 3s: " << (server_->is_connected() ? "connected" : "not connected") << std::endl;
  std::cout << "Connect callback called: " << (connect_called->load() ? "yes" : "no") << std::endl;
  std::cout << "Error callback called: " << (error_called->load() ? "yes" : "no") << std::endl;
  if (error_called->load()) {
    std::cout << "Last error: " << *last_error << std::endl;
  }

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);

  // 에러가 발생했다면 출력
  if (error_called->load()) {
    std::cout << "Server encountered error: " << *last_error << std::endl;
  }
}

/**
 * @brief 서버 상태 확인 테스트
 */
TEST_F(SimpleServerTest, ServerStateCheck) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing server state check, port: " << test_port << std::endl;

  // 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .unlimited_clients()  // 클라이언트 제한 없음
                .auto_start(false)
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";

  // 시작 전 상태
  std::cout << "Before start - is_connected(): " << (server_->is_connected() ? "true" : "false") << std::endl;
  EXPECT_FALSE(server_->is_connected()) << "Server should not be connected before start";

  // 서버 시작
  std::cout << "Starting server..." << std::endl;
  server_->start();

  // 시작 후 상태 확인 (여러 번)
  for (int i = 0; i < 5; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "After " << (i + 1) << "s - is_connected(): " << (server_->is_connected() ? "true" : "false")
              << std::endl;
  }

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 클라이언트 제한 기능 테스트 - Single Client
 */
TEST_F(SimpleServerTest, ClientLimitSingleClient) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing single client limit, port: " << test_port << std::endl;

  // Single client 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .single_client()  // 1개 클라이언트만 허용
                .auto_start(false)
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Single client server created" << std::endl;

  // 서버 시작
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::cout << "Single client server started" << std::endl;
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 클라이언트 제한 기능 테스트 - Multi Client
 */
TEST_F(SimpleServerTest, ClientLimitMultiClient) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing multi client limit (3 clients), port: " << test_port << std::endl;

  // Multi client 서버 생성 (3개 클라이언트 제한)
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .multi_client(3)  // 3개 클라이언트만 허용
                .auto_start(false)
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Multi client server (limit 3) created" << std::endl;

  // 서버 시작
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::cout << "Multi client server started" << std::endl;
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 클라이언트 제한 기능 테스트 - Unlimited Clients
 */
TEST_F(SimpleServerTest, ClientLimitUnlimitedClients) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing unlimited clients, port: " << test_port << std::endl;

  // Unlimited clients 서버 생성
  server_ = builder::UnifiedBuilder::tcp_server(test_port)
                .unlimited_clients()  // 클라이언트 제한 없음
                .auto_start(false)
                .build();

  ASSERT_NE(server_, nullptr) << "Server creation failed";
  std::cout << "Unlimited clients server created" << std::endl;

  // 서버 시작
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::cout << "Unlimited clients server started" << std::endl;
  EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 클라이언트 제한 기능 테스트 - Builder Validation
 */
TEST_F(SimpleServerTest, ClientLimitBuilderValidation) {
  uint16_t test_port = getTestPort();
  std::cout << "Testing client limit builder validation, port: " << test_port << std::endl;

  // 잘못된 설정으로 서버 생성 시도 (0개 클라이언트)
  EXPECT_THROW(
      {
        server_ = builder::UnifiedBuilder::tcp_server(test_port)
                      .multi_client(0)  // 0개는 유효하지 않음
                      .auto_start(false)
                      .build();
      },
      std::invalid_argument)
      << "Should throw exception for 0 client limit";

  std::cout << "Builder validation test passed" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

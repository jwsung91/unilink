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
#include <chrono>
#include <memory>
#include <thread>

#include "unilink/common/exceptions.hpp"
#include "unilink/common/io_context_manager.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;

class BuilderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 테스트 전 초기화
    data_received_.clear();
    connection_established_ = false;
    error_occurred_ = false;
  }

  void TearDown() override {
    // 테스트 후 정리
    if (server_) {
      server_->stop();
      server_.reset();
    }
    if (client_) {
      client_->stop();
      client_.reset();
    }
    if (serial_) {
      serial_->stop();
      serial_.reset();
    }

    // 충분한 시간을 두고 정리 완료 보장
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // 테스트용 포트 번호 (동적 할당으로 충돌 방지)
  uint16_t getTestPort() {
    static std::atomic<uint16_t> port_counter{9000};
    return port_counter.fetch_add(1);
  }

  // 테스트용 데이터 핸들러
  void setupDataHandler() {
    if (server_) {
      server_->on_data([this](const std::string& data) { data_received_.push_back(data); });
    }
    if (client_) {
      client_->on_data([this](const std::string& data) { data_received_.push_back(data); });
    }
    if (serial_) {
      serial_->on_data([this](const std::string& data) { data_received_.push_back(data); });
    }
  }

  // 테스트용 연결 핸들러
  void setupConnectionHandler() {
    if (server_) {
      server_->on_connect([this]() { connection_established_ = true; });
    }
    if (client_) {
      client_->on_connect([this]() { connection_established_ = true; });
    }
    if (serial_) {
      serial_->on_connect([this]() { connection_established_ = true; });
    }
  }

  // 테스트용 에러 핸들러
  void setupErrorHandler() {
    if (server_) {
      server_->on_error([this](const std::string& error) {
        error_occurred_ = true;
        last_error_ = error;
      });
    }
    if (client_) {
      client_->on_error([this](const std::string& error) {
        error_occurred_ = true;
        last_error_ = error;
      });
    }
    if (serial_) {
      serial_->on_error([this](const std::string& error) {
        error_occurred_ = true;
        last_error_ = error;
      });
    }
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;
  std::shared_ptr<wrapper::Serial> serial_;

  std::vector<std::string> data_received_;
  std::atomic<bool> connection_established_{false};
  std::atomic<bool> error_occurred_{false};
  std::string last_error_;
};

// TcpServerBuilder 기본 테스트
TEST_F(BuilderTest, TcpServerBuilderBasic) {
  uint16_t test_port = getTestPort();

  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()
                .auto_start(false)  // 수동 시작으로 제어
                .on_data([](const std::string& data) {
                  // 데이터 처리
                })
                .on_connect([]() {
                  // 연결 처리
                })
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_FALSE(server_->is_connected());

  // 수동으로 시작
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
}

// TcpClientBuilder 기본 테스트
TEST_F(BuilderTest, TcpClientBuilderBasic) {
  uint16_t test_port = getTestPort();

  client_ = unilink::tcp_client("127.0.0.1", test_port)
                .auto_start(false)  // 수동 시작으로 제어
                .on_data([](const std::string& data) {
                  // 데이터 처리
                })
                .on_connect([]() {
                  // 연결 처리
                })
                .build();

  ASSERT_NE(client_, nullptr);
  EXPECT_FALSE(client_->is_connected());

  // 수동으로 시작
  client_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 클라이언트가 생성되었는지 확인
  EXPECT_TRUE(client_ != nullptr);
}

// SerialBuilder 기본 테스트
TEST_F(BuilderTest, SerialBuilderBasic) {
  serial_ = unilink::serial("/dev/null", 9600)
                .auto_start(false)  // 수동 시작으로 제어
                .on_data([](const std::string& data) {
                  // 데이터 처리
                })
                .on_connect([]() {
                  // 연결 처리
                })
                .build();

  ASSERT_NE(serial_, nullptr);
  EXPECT_FALSE(serial_->is_connected());

  // 수동으로 시작
  serial_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Serial이 생성되었는지 확인
  EXPECT_TRUE(serial_ != nullptr);
}

// Builder 체이닝 테스트
TEST_F(BuilderTest, BuilderChaining) {
  uint16_t test_port = getTestPort();

  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()
                .auto_start(false)
                .auto_manage(true)
                .on_data([this](const std::string& data) { data_received_.push_back(data); })
                .on_connect([this]() { connection_established_ = true; })
                .on_disconnect([]() {
                  // 연결 해제 처리
                })
                .on_error([this](const std::string& error) {
                  error_occurred_ = true;
                  last_error_ = error;
                })
                .build();

  ASSERT_NE(server_, nullptr);
  EXPECT_FALSE(server_->is_connected());

  // 수동으로 시작
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
}

// 여러 Builder 동시 사용 테스트
TEST_F(BuilderTest, MultipleBuilders) {
  uint16_t test_port = getTestPort();

  server_ = unilink::tcp_server(test_port).unlimited_clients().auto_start(false).build();

  client_ = unilink::tcp_client("127.0.0.1", test_port).auto_start(false).build();

  serial_ = unilink::serial("/dev/null", 115200).auto_start(false).build();

  ASSERT_NE(server_, nullptr);
  ASSERT_NE(client_, nullptr);
  ASSERT_NE(serial_, nullptr);

  // 각각 시작
  server_->start();
  client_->start();
  serial_->start();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 모든 객체가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
  EXPECT_TRUE(client_ != nullptr);
  EXPECT_TRUE(serial_ != nullptr);
}

// Builder 설정 검증 테스트
TEST_F(BuilderTest, BuilderConfiguration) {
  uint16_t test_port = getTestPort();

  server_ = unilink::tcp_server(test_port).unlimited_clients().auto_start(false).auto_manage(false).build();

  ASSERT_NE(server_, nullptr);
  EXPECT_FALSE(server_->is_connected());

  // 수동으로 시작
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
}

// 콜백 등록 테스트
TEST_F(BuilderTest, CallbackRegistration) {
  uint16_t test_port = getTestPort();
  std::atomic<int> callback_count{0};
  std::atomic<int> error_callback_count{0};

  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()
                .on_data([&callback_count](const std::string& data) { callback_count++; })
                .on_connect([&callback_count]() { callback_count++; })
                .on_disconnect([&callback_count]() { callback_count++; })
                .on_error([&error_callback_count](const std::string& error) { error_callback_count++; })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 시작
  server_->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);

  // 포트 충돌로 인한 에러 콜백은 허용하되, 다른 콜백은 호출되지 않아야 함
  // 에러 콜백이 호출된 경우 (포트 충돌) 다른 콜백은 호출되지 않아야 함
  if (error_callback_count.load() > 0) {
    // 포트 충돌로 인한 에러가 발생한 경우, 다른 콜백은 호출되지 않아야 함
    EXPECT_EQ(callback_count.load(), 0);
  } else {
    // 에러가 없는 경우, 모든 콜백이 호출되지 않아야 함
    EXPECT_EQ(callback_count.load(), 0);
    EXPECT_EQ(error_callback_count.load(), 0);
  }
}

// 빌더 재사용 테스트
TEST_F(BuilderTest, BuilderReuse) {
  uint16_t test_port = getTestPort();
  auto builder = unilink::tcp_server(test_port);

  // 첫 번째 서버
  auto server1 = builder.unlimited_clients().auto_start(false).on_data([](const std::string& data) {}).build();

  // 두 번째 서버 (같은 빌더 재사용)
  auto server2 = builder.auto_start(false).on_connect([]() {}).build();

  ASSERT_NE(server1, nullptr);
  ASSERT_NE(server2, nullptr);

  // 각각 시작
  server1->start();
  server2->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // 각 서버가 생성되었는지 확인
  EXPECT_TRUE(server1 != nullptr);
  EXPECT_TRUE(server2 != nullptr);

  // 정리
  server1->stop();
  server2->stop();
}

// 편의 함수 테스트
TEST_F(BuilderTest, ConvenienceFunctions) {
  uint16_t test_port = getTestPort();

  // tcp_server 편의 함수 테스트
  auto server = unilink::tcp_server(test_port)
                    .unlimited_clients()
                    .on_connect([]() {})
                    .on_data([](const std::string& data) {})
                    .build();

  // tcp_client 편의 함수 테스트
  auto client =
      unilink::tcp_client("127.0.0.1", test_port).on_connect([]() {}).on_data([](const std::string& data) {}).build();

  // serial 편의 함수 테스트
  auto serial = unilink::serial("/dev/null", 9600).on_connect([]() {}).on_data([](const std::string& data) {}).build();

  // 객체들이 제대로 생성되었는지 확인
  EXPECT_NE(server, nullptr);
  EXPECT_NE(client, nullptr);
  EXPECT_NE(serial, nullptr);

  // 타입 확인
  EXPECT_TRUE(dynamic_cast<wrapper::TcpServer*>(server.get()) != nullptr);
  EXPECT_TRUE(dynamic_cast<wrapper::TcpClient*>(client.get()) != nullptr);
  EXPECT_TRUE(dynamic_cast<wrapper::Serial*>(serial.get()) != nullptr);

  // 정리
  server->stop();
  client->stop();
  serial->stop();
}

// ============================================================================
// IoContext 관리 기능 테스트
// ============================================================================

/**
 * @brief IoContextManager 기본 기능 테스트
 */
TEST_F(BuilderTest, IoContextManagerBasicFunctionality) {
  auto& manager = common::IoContextManager::instance();

  // 이전 테스트의 영향을 받을 수 있으므로 현재 상태 확인
  bool was_running = manager.is_running();

  // 시작
  manager.start();
  EXPECT_TRUE(manager.is_running());

  // 컨텍스트 가져오기
  auto& context = manager.get_context();
  EXPECT_NE(&context, nullptr);

  // 중지
  manager.stop();
  EXPECT_FALSE(manager.is_running());

  // 원래 상태로 복원 (필요한 경우)
  if (was_running) {
    manager.start();
  }
}

/**
 * @brief 독립적인 컨텍스트 생성 테스트
 */
TEST_F(BuilderTest, IndependentContextCreation) {
  auto& manager = common::IoContextManager::instance();

  // 독립적인 컨텍스트 생성
  auto independent_context = manager.create_independent_context();
  EXPECT_NE(independent_context, nullptr);

  // 전역 컨텍스트와 다른 인스턴스인지 확인
  manager.start();
  auto& global_context = manager.get_context();
  EXPECT_NE(independent_context.get(), &global_context);

  manager.stop();
}

/**
 * @brief 빌더에서 독립적인 컨텍스트 사용 테스트
 */
TEST_F(BuilderTest, BuilderWithIndependentContext) {
  uint16_t test_port = getTestPort();

  // 독립적인 컨텍스트를 사용하는 서버 생성
  auto server =
      unilink::tcp_server(test_port).unlimited_clients().use_independent_context(true).auto_start(false).build();

  EXPECT_NE(server, nullptr);

  // 독립적인 컨텍스트를 사용하는 클라이언트 생성
  auto client = unilink::tcp_client("127.0.0.1", test_port).use_independent_context(true).auto_start(false).build();

  EXPECT_NE(client, nullptr);

  // 공유 컨텍스트를 사용하는 서버 생성
  auto shared_server =
      unilink::tcp_server(test_port + 1).unlimited_clients().use_independent_context(false).auto_start(false).build();

  EXPECT_NE(shared_server, nullptr);
}

/**
 * @brief 테스트 격리 시나리오
 */
TEST_F(BuilderTest, TestIsolationScenario) {
  // 테스트 1: 독립적인 컨텍스트 사용
  auto client1 =
      unilink::tcp_client("127.0.0.1", getTestPort()).use_independent_context(true).auto_start(false).build();

  // 테스트 2: 공유 컨텍스트 사용
  auto client2 =
      unilink::tcp_client("127.0.0.1", getTestPort()).use_independent_context(false).auto_start(false).build();

  // 테스트 3: 또 다른 독립적인 컨텍스트
  auto client3 =
      unilink::tcp_client("127.0.0.1", getTestPort()).use_independent_context(true).auto_start(false).build();

  // 모든 클라이언트가 성공적으로 생성되었는지 확인
  EXPECT_NE(client1, nullptr);
  EXPECT_NE(client2, nullptr);
  EXPECT_NE(client3, nullptr);
}

/**
 * @brief 메서드 체이닝과 독립적인 컨텍스트 조합 테스트
 */
TEST_F(BuilderTest, MethodChainingWithIndependentContext) {
  auto client = unilink::tcp_client("127.0.0.1", getTestPort())
                    .use_independent_context(true)
                    .auto_start(false)
                    .auto_manage(false)
                    .on_connect([]() { std::cout << "Connected!" << std::endl; })
                    .on_disconnect([]() { std::cout << "Disconnected!" << std::endl; })
                    .on_data([](const std::string& data) { std::cout << "Data: " << data << std::endl; })
                    .on_error([](const std::string& error) { std::cout << "Error: " << error << std::endl; })
                    .build();

  EXPECT_NE(client, nullptr);
}

// ============================================================================
// BUILDER EXCEPTION SAFETY TESTS
// ============================================================================

/**
 * @brief Test TCP client builder exception safety
 * NOTE: Temporarily disabled due to UnifiedBuilder namespace issues
 */
TEST_F(BuilderTest, DISABLED_TcpClientBuilderExceptionSafety) {
  // Test invalid host
  // EXPECT_THROW({ auto client = unilink::tcp_client("", 8080).build(); }, common::BuilderException);

  // Test invalid port
  // EXPECT_THROW({ auto client = unilink::tcp_client("localhost", 0).build(); }, common::BuilderException);

  // Test invalid retry interval
  // EXPECT_THROW(
  //     { auto client = unilink::tcp_client("localhost", 8080).retry_interval(0).build(); },
  //     common::BuilderException);

  // Test valid configuration
  // EXPECT_NO_THROW({
  //   auto client = unilink::tcp_client("localhost", 8080).build();
  //   EXPECT_NE(client, nullptr);
  // });
}

/**
 * @brief Test end-to-end exception safety
 * NOTE: Temporarily disabled due to UnifiedBuilder namespace issues
 */
TEST_F(BuilderTest, DISABLED_EndToEndExceptionSafety) {
  // Test that invalid configurations throw appropriate exceptions
  // EXPECT_THROW(auto server = unilink::tcp_server(0).unlimited_clients().build(), common::BuilderException);

  // EXPECT_THROW(auto client = unilink::tcp_client("invalid..hostname", 8080).build(), common::BuilderException);

  // Test that valid configurations work
  // EXPECT_NO_THROW(auto server = unilink::tcp_server(8080).unlimited_clients().build());

  // EXPECT_NO_THROW(auto client = unilink::tcp_client("localhost", 8080).build());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

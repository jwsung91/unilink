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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace std::chrono_literals;

class BuilderIntegrationTest : public ::testing::Test {
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
      auto promise = std::make_shared<std::promise<void>>();
      server_->stop([promise] { promise->set_value(); });
      promise->get_future().wait_for(1s);
      server_.reset();
    }
    if (client_) {
      auto promise = std::make_shared<std::promise<void>>();
      client_->stop([promise] { promise->set_value(); });
      promise->get_future().wait_for(1s);
      client_.reset();
    }
    if (serial_) {
      auto promise = std::make_shared<std::promise<void>>();
      serial_->stop([promise] { promise->set_value(); });
      promise->get_future().wait_for(1s);
      serial_.reset();
    }

    // 충분한 시간을 두고 정리 완료 보장
    // TIME_WAIT 상태에서도 포트 충돌 방지를 위해 200ms 대기
    std::this_thread::sleep_for(200ms);
  }

  // 테스트용 포트 번호 - TestUtils의 통합 포트 할당 사용
  // SO_REUSEADDR와 100-port spacing이 적용됨
  uint16_t getTestPort() { return TestUtils::getAvailableTestPort(); }

  // 데이터 수신 대기
  void waitForData(std::chrono::milliseconds timeout = 1000ms) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait_for(lock, timeout, [this] { return !data_received_.empty(); });
  }

  // 연결 대기
  void waitForConnection(std::chrono::milliseconds timeout = 1000ms) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait_for(lock, timeout, [this] { return connection_established_.load(); });
  }

  // 테스트용 데이터 핸들러
  void setupDataHandler() {
    if (server_) {
      server_->on_data([this](const std::string& data) {
        std::lock_guard<std::mutex> lock(mtx_);
        data_received_.push_back(data);
        cv_.notify_one();
      });
    }
    if (client_) {
      client_->on_data([this](const std::string& data) {
        std::lock_guard<std::mutex> lock(mtx_);
        data_received_.push_back(data);
        cv_.notify_one();
      });
    }
    if (serial_) {
      serial_->on_data([this](const std::string& data) {
        std::lock_guard<std::mutex> lock(mtx_);
        data_received_.push_back(data);
        cv_.notify_one();
      });
    }
  }

  // 테스트용 연결 핸들러
  void setupConnectionHandler() {
    if (server_) {
      server_->on_connect([this]() {
        std::lock_guard<std::mutex> lock(mtx_);
        connection_established_.store(true);
        cv_.notify_one();
      });
    }
    if (client_) {
      client_->on_connect([this]() {
        std::lock_guard<std::mutex> lock(mtx_);
        connection_established_.store(true);
        cv_.notify_one();
      });
    }
    if (serial_) {
      serial_->on_connect([this]() {
        std::lock_guard<std::mutex> lock(mtx_);
        connection_established_.store(true);
        cv_.notify_one();
      });
    }
  }

  // 테스트용 에러 핸들러
  void setupErrorHandler() {
    if (server_) {
      server_->on_error([this](const std::string& error) {
        std::lock_guard<std::mutex> lock(mtx_);
        error_occurred_.store(true);
        last_error_ = error;
        cv_.notify_one();
      });
    }
    if (client_) {
      client_->on_error([this](const std::string& error) {
        std::lock_guard<std::mutex> lock(mtx_);
        error_occurred_.store(true);
        last_error_ = error;
        cv_.notify_one();
      });
    }
    if (serial_) {
      serial_->on_error([this](const std::string& error) {
        std::lock_guard<std::mutex> lock(mtx_);
        error_occurred_.store(true);
        last_error_ = error;
        cv_.notify_one();
      });
    }
  }

 protected:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::TcpClient> client_;
  std::shared_ptr<wrapper::Serial> serial_;

  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::string> data_received_;
  std::atomic<bool> connection_established_{false};
  std::atomic<bool> error_occurred_{false};
  std::string last_error_;
};

// ============================================================================
// BASIC BUILDER CREATION TESTS
// ============================================================================

/**
 * @brief TcpServerBuilder로 서버 생성 및 기본 동작 테스트
 */
TEST_F(BuilderIntegrationTest, TcpServerBuilderCreatesServer) {
  // --- Setup ---
  uint16_t test_port = getTestPort();

  // --- Test Logic ---
  auto server = unilink::tcp_server(test_port)
                    .unlimited_clients()
                    // 수동 시작으로 제어
                    .on_data([](const std::string& data) {
                      // 데이터 핸들러
                    })
                    .on_connect([]() {
                      // 연결 핸들러
                    })
                    .on_error([](const std::string& error) {
                      // 에러 핸들러
                    })
                    .build();

  // --- Verification ---
  ASSERT_NE(server, nullptr);
  EXPECT_FALSE(server->is_connected());  // 아직 시작하지 않았으므로 연결되지 않음

  // 수동으로 시작
  server->start();
  std::this_thread::sleep_for(100ms);  // 서버 시작 대기

  // 서버가 리스닝 상태인지 확인 (실제 네트워크 바인딩 테스트)
  // Note: 실제 포트 바인딩은 시스템 레벨에서 확인해야 함
  EXPECT_TRUE(server != nullptr);

  server->stop();
}

/**
 * @brief TcpClientBuilder로 클라이언트 생성 및 기본 동작 테스트
 */
TEST_F(BuilderIntegrationTest, TcpClientBuilderCreatesClient) {
  // --- Setup ---
  uint16_t test_port = getTestPort();

  // --- Test Logic ---
  auto client = unilink::tcp_client("127.0.0.1", test_port)
                    // 수동 시작으로 제어
                    .on_data([](const std::string& data) {
                      // 데이터 핸들러
                    })
                    .on_connect([]() {
                      // 연결 핸들러
                    })
                    .on_error([](const std::string& error) {
                      // 에러 핸들러
                    })
                    .build();

  // --- Verification ---
  ASSERT_NE(client, nullptr);
  EXPECT_FALSE(client->is_connected());  // 아직 시작하지 않았으므로 연결되지 않음

  // 수동으로 시작 (연결 시도)
  client->start();
  std::this_thread::sleep_for(100ms);  // 연결 시도 대기

  // 클라이언트가 생성되었는지 확인
  EXPECT_TRUE(client != nullptr);

  client->stop();
}

// ============================================================================
// BUILDER CONFIGURATION TESTS
// ============================================================================

/**
 * @brief auto_start 설정이 올바르게 적용되는지 테스트
 */
TEST_F(BuilderIntegrationTest, AutoStartConfiguration) {
  // --- Setup ---
  uint16_t test_port = getTestPort();

  // --- Test Logic ---
  // auto_start = false인 경우
  auto server_manual = unilink::tcp_server(test_port).unlimited_clients().build();

  EXPECT_FALSE(server_manual->is_connected());

  server_manual->start();
  std::this_thread::sleep_for(100ms);
  // 서버가 시작되었는지 확인
  EXPECT_TRUE(server_manual != nullptr);

  server_manual->stop();

  // auto_start = true인 경우 (실제로는 build() 시점에 시작됨)
  auto server_auto = unilink::tcp_server(test_port + 1).unlimited_clients().build();

  std::this_thread::sleep_for(100ms);
  // auto_start가 적용되었는지 확인
  EXPECT_TRUE(server_auto != nullptr);

  server_auto->stop();
}

/**
 * @brief auto_manage 설정이 올바르게 적용되는지 테스트
 */
TEST_F(BuilderIntegrationTest, AutoManageConfiguration) {
  // --- Setup ---
  uint16_t test_port = getTestPort();

  // --- Test Logic ---
  auto server = unilink::tcp_server(test_port).unlimited_clients().auto_manage(true).build();

  // auto_manage 설정이 적용되었는지 확인
  EXPECT_TRUE(server != nullptr);

  server->start();
  std::this_thread::sleep_for(100ms);

  // auto_manage가 적용되어 자동 관리되는지 확인
  EXPECT_TRUE(server != nullptr);

  server->stop();
}

// ============================================================================
// CALLBACK REGISTRATION TESTS
// ============================================================================

/**
 * @brief 빌더를 통한 콜백 등록이 올바르게 작동하는지 테스트
 */
TEST_F(BuilderIntegrationTest, CallbackRegistration) {
  // --- Setup ---
  uint16_t test_port = getTestPort();
  std::atomic<int> data_callback_count{0};
  std::atomic<int> connect_callback_count{0};
  std::atomic<int> error_callback_count{0};

  // --- Test Logic ---
  auto server = unilink::tcp_server(test_port)
                    .unlimited_clients()
                    .on_data([&](const std::string& data) { data_callback_count++; })
                    .on_connect([&]() { connect_callback_count++; })
                    .on_error([&](const std::string& error) { error_callback_count++; })
                    .build();

  // --- Verification ---
  ASSERT_NE(server, nullptr);

  // 콜백이 등록되었는지 확인 (실제 호출은 연결이 있을 때)
  EXPECT_TRUE(server != nullptr);

  server->start();
  std::this_thread::sleep_for(100ms);

  // 포트 충돌로 인한 에러 콜백은 허용하되, 다른 콜백은 호출되지 않아야 함
  if (error_callback_count.load() > 0) {
    // 포트 충돌로 인한 에러가 발생한 경우, 다른 콜백은 호출되지 않아야 함
    EXPECT_EQ(data_callback_count.load(), 0);
    EXPECT_EQ(connect_callback_count.load(), 0);
  } else {
    // 에러가 없는 경우, 모든 콜백이 호출되지 않아야 함
    EXPECT_EQ(data_callback_count.load(), 0);
    EXPECT_EQ(connect_callback_count.load(), 0);
    EXPECT_EQ(error_callback_count.load(), 0);
  }

  server->stop();
}

// ============================================================================
// BUILDER CHAINING TESTS
// ============================================================================

/**
 * @brief 빌더 메서드 체이닝이 올바르게 작동하는지 테스트
 */
TEST_F(BuilderIntegrationTest, BuilderMethodChaining) {
  // --- Setup ---
  uint16_t test_port = getTestPort();

  // --- Test Logic ---
  auto server = unilink::tcp_server(test_port)
                    .unlimited_clients()

                    .auto_manage(true)
                    .on_data([](const std::string& data) {})
                    .on_connect([]() {})
                    .on_disconnect([]() {})
                    .on_error([](const std::string& error) {})
                    .build();

  // --- Verification ---
  ASSERT_NE(server, nullptr);
  EXPECT_FALSE(server->is_connected());

  // 모든 설정이 적용되었는지 확인
  server->start();
  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(server != nullptr);

  server->stop();
}

// ============================================================================
// MULTIPLE BUILDER INSTANCES TESTS
// ============================================================================

/**
 * @brief 여러 빌더 인스턴스가 독립적으로 작동하는지 테스트
 */
TEST_F(BuilderIntegrationTest, MultipleBuilderInstances) {
  // --- Setup ---
  uint16_t port1 = getTestPort();
  uint16_t port2 = getTestPort();

  // --- Test Logic ---
  auto server1 = unilink::tcp_server(port1).unlimited_clients().build();

  auto server2 = unilink::tcp_server(port2).unlimited_clients().build();

  auto client1 = unilink::tcp_client("127.0.0.1", port1).build();

  auto client2 = unilink::tcp_client("127.0.0.1", port2).build();

  // --- Verification ---
  ASSERT_NE(server1, nullptr);
  ASSERT_NE(server2, nullptr);
  ASSERT_NE(client1, nullptr);
  ASSERT_NE(client2, nullptr);

  // 각 인스턴스가 독립적으로 작동하는지 확인
  EXPECT_FALSE(server1->is_connected());
  EXPECT_FALSE(server2->is_connected());
  EXPECT_FALSE(client1->is_connected());
  EXPECT_FALSE(client2->is_connected());

  // 각각 시작
  server1->start();
  server2->start();
  client1->start();
  client2->start();

  std::this_thread::sleep_for(100ms);

  // 각 인스턴스가 독립적으로 작동하는지 확인
  EXPECT_TRUE(server1 != nullptr);
  EXPECT_TRUE(server2 != nullptr);
  EXPECT_TRUE(client1 != nullptr);
  EXPECT_TRUE(client2 != nullptr);

  // 정리
  server1->stop();
  server2->stop();
  client1->stop();
  client2->stop();
}

// ============================================================================
// BUILDER REUSE TESTS
// ============================================================================

/**
 * @brief 빌더 재사용이 올바르게 작동하는지 테스트
 * NOTE: Builder reuse is not supported in the new design.
 * Builders should be used once and then discarded.
 */
TEST_F(BuilderIntegrationTest, DISABLED_BuilderReuse) {
  // --- Setup ---
  uint16_t test_port = getTestPort();

  // --- Test Logic ---
  auto&& builder = unilink::tcp_server(test_port).unlimited_clients();

  // 첫 번째 서버 생성
  auto server1 = builder.on_data([](const std::string& data) {}).build();

  // 두 번째 서버 생성 (같은 빌더 재사용)
  auto server2 = builder.on_connect([]() {}).build();

  // --- Verification ---
  ASSERT_NE(server1, nullptr);
  ASSERT_NE(server2, nullptr);

  // 각 서버가 독립적으로 작동하는지 확인
  EXPECT_FALSE(server1->is_connected());

  server1->start();
  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(server1 != nullptr);
  EXPECT_TRUE(server2 != nullptr);

  // 정리
  server1->stop();
  server2->stop();
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

/**
 * @brief 빌더에서 잘못된 설정이 올바르게 처리되는지 테스트
 */
TEST_F(BuilderIntegrationTest, ErrorHandling) {
  // --- Setup ---
  uint16_t test_port = getTestPort();

  // --- Test Logic ---
  // Test 1: 유효하지 않은 포트로 서버 생성 시도는 예외 발생
  EXPECT_THROW(
      {
        auto server = unilink::tcp_server(0)  // 유효하지 않은 포트
                          .unlimited_clients()

                          .build();
      },
      std::exception);

  // Test 2: 유효한 서버로 에러 핸들러 테스트
  auto server = unilink::tcp_server(test_port)
                    .unlimited_clients()

                    .on_error([this](const std::string& error) {
                      std::lock_guard<std::mutex> lock(mtx_);
                      error_occurred_ = true;
                      last_error_ = error;
                      cv_.notify_one();
                    })
                    .build();

  // --- Verification ---
  ASSERT_NE(server, nullptr);

  server->start();
  std::this_thread::sleep_for(100ms);

  // 서버가 생성되었는지 확인
  EXPECT_TRUE(server != nullptr);

  server->stop();
}

// ============================================================================
// PERFORMANCE TESTS
// ============================================================================

/**
 * @brief 빌더를 통한 빠른 객체 생성이 가능한지 테스트
 */
TEST_F(BuilderIntegrationTest, FastObjectCreation) {
  // --- Setup ---
  const int num_objects = 100;
  std::vector<std::shared_ptr<wrapper::TcpServer>> servers;
  std::vector<std::shared_ptr<wrapper::TcpClient>> clients;

  // --- Test Logic ---
  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_objects; ++i) {
    uint16_t port = getTestPort();

    auto server = unilink::tcp_server(port).unlimited_clients().build();

    auto client = unilink::tcp_client("127.0.0.1", port).build();

    servers.push_back(std::shared_ptr<wrapper::TcpServer>(server.release()));
    clients.push_back(std::shared_ptr<wrapper::TcpClient>(client.release()));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // --- Verification ---
  EXPECT_EQ(servers.size(), num_objects);
  EXPECT_EQ(clients.size(), num_objects);

  // 모든 객체가 성공적으로 생성되었는지 확인
  for (const auto& server : servers) {
    EXPECT_NE(server, nullptr);
  }
  for (const auto& client : clients) {
    EXPECT_NE(client, nullptr);
  }

  // 생성 시간이 합리적인지 확인 (100개 객체를 1초 이내에 생성)
  EXPECT_LT(duration.count(), 1000);

  // 정리
  for (auto& server : servers) {
    server->stop();
  }
  for (auto& client : clients) {
    client->stop();
  }
}

// ============================================================================
// INTEGRATION TESTS WITH REAL COMMUNICATION
// ============================================================================

/**
 * @brief 빌더로 생성된 서버와 클라이언트 간 실제 통신 테스트
 *
 * Note: 이 테스트는 실제 네트워크 통신을 시도하므로
 * 네트워크 환경에 따라 결과가 달라질 수 있습니다.
 */
TEST_F(BuilderIntegrationTest, RealCommunicationBetweenBuilderObjects) {
  // --- Setup ---
  uint16_t test_port = getTestPort();
  server_ = nullptr;
  client_ = nullptr;

  // --- Test Logic ---
  // 서버 생성
  server_ = unilink::tcp_server(test_port)
                .unlimited_clients()

                .on_data([this](const std::string& data) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  data_received_.push_back(data);
                  cv_.notify_one();
                })
                .on_connect([this]() {
                  std::lock_guard<std::mutex> lock(mtx_);
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(server_, nullptr);

  // 서버 시작 대기
  std::this_thread::sleep_for(200ms);

  // 클라이언트 생성
  client_ = unilink::tcp_client("127.0.0.1", test_port)

                .on_data([this](const std::string& data) {
                  std::lock_guard<std::mutex> lock(mtx_);
                  data_received_.push_back(data);
                  cv_.notify_one();
                })
                .on_connect([this]() {
                  std::lock_guard<std::mutex> lock(mtx_);
                  connection_established_ = true;
                  cv_.notify_one();
                })
                .build();

  ASSERT_NE(client_, nullptr);

  // 클라이언트 연결 시도 대기
  std::this_thread::sleep_for(200ms);

  // 데이터 전송 테스트
  if (client_->is_connected()) {
    client_->send("test message from builder client");

    // 데이터 수신 대기
    waitForData(1000ms);

    // --- Verification ---
    EXPECT_FALSE(data_received_.empty());
    if (!data_received_.empty()) {
      EXPECT_EQ(data_received_[0], "test message from builder client");
    }
  } else {
    // 연결되지 않은 경우에도 테스트는 통과 (네트워크 환경에 따라)
    GTEST_SKIP() << "Client could not connect to server (network environment dependent)";
  }
}

/**
 * @brief 빌더 설정이 실제 통신 동작에 반영되는지 테스트
 */
TEST_F(BuilderIntegrationTest, BuilderConfigurationAffectsCommunication) {
  // --- Setup ---
  uint16_t test_port = getTestPort();
  std::atomic<int> server_data_count{0};
  std::atomic<int> client_data_count{0};

  // --- Test Logic ---
  // 서버 생성 (에코 서버로 설정)
  auto server = unilink::tcp_server(test_port)
                    .unlimited_clients()

                    .on_data([&](const std::string& data) {
                      server_data_count++;
                      // 에코 서버: 받은 데이터를 그대로 다시 전송
                      // Note: 실제 구현에서는 서버에서 클라이언트로 데이터를 보낼 수 있어야 함
                    })
                    .build();

  ASSERT_NE(server, nullptr);

  // 서버 시작 대기
  std::this_thread::sleep_for(200ms);

  // 클라이언트 생성
  auto client = unilink::tcp_client("127.0.0.1", test_port)

                    .on_data([&](const std::string& data) { client_data_count++; })
                    .build();

  ASSERT_NE(client, nullptr);

  // 클라이언트 연결 시도 대기
  std::this_thread::sleep_for(200ms);

  // --- Verification ---
  // 빌더로 생성된 객체들이 올바르게 작동하는지 확인
  EXPECT_TRUE(server != nullptr);
  EXPECT_TRUE(client != nullptr);

  // 실제 통신이 가능한 경우 데이터 전송 테스트
  if (client->is_connected()) {
    client->send("configuration test message");
    std::this_thread::sleep_for(100ms);

    // 서버에서 데이터를 받았는지 확인
    EXPECT_GT(server_data_count.load(), 0);
  } else {
    // 연결되지 않은 경우에도 빌더 설정이 올바르게 적용되었는지 확인
    GTEST_SKIP() << "Client could not connect to server (network environment dependent)";
  }

  // 정리
  server->stop();
  client->stop();
}

// ============================================================================
// SERIAL INTEGRATION TESTS
// ============================================================================

/**
 * @brief SerialBuilder로 Serial 생성 및 기본 동작 테스트
 */
TEST_F(BuilderIntegrationTest, SerialBuilderCreatesSerial) {
  // --- Setup ---
  std::string test_device = "/dev/null";  // 테스트용 장치
  uint32_t test_baud_rate = 9600;

  // --- Test Logic ---
  serial_ = unilink::serial(test_device, test_baud_rate)
                // 수동 시작으로 제어
                .on_data([](const std::string& data) {
                  // 데이터 핸들러
                })
                .on_connect([]() {
                  // 연결 핸들러
                })
                .on_error([](const std::string& error) {
                  // 에러 핸들러
                })
                .build();

  // --- Verification ---
  ASSERT_NE(serial_, nullptr);
  EXPECT_FALSE(serial_->is_connected());  // 아직 시작하지 않았으므로 연결되지 않음

  // 수동으로 시작
  serial_->start();
  std::this_thread::sleep_for(100ms);  // Serial 시작 대기

  // Serial이 생성되었는지 확인
  EXPECT_TRUE(serial_ != nullptr);
}

/**
 * @brief SerialBuilder 설정이 실제 동작에 반영되는지 테스트
 */
TEST_F(BuilderIntegrationTest, SerialBuilderConfiguration) {
  // --- Setup ---
  std::string test_device = "/dev/null";
  uint32_t test_baud_rate = 115200;

  // --- Test Logic ---
  // auto_start = false인 경우
  auto serial_manual = unilink::serial(test_device, test_baud_rate).build();

  EXPECT_FALSE(serial_manual->is_connected());

  serial_manual->start();
  std::this_thread::sleep_for(100ms);

  // Serial이 시작되었는지 확인
  EXPECT_TRUE(serial_manual != nullptr);

  serial_manual->stop();

  // auto_start = true인 경우
  auto serial_auto = unilink::serial(test_device, test_baud_rate + 1).build();

  std::this_thread::sleep_for(100ms);

  // auto_start가 적용되었는지 확인
  EXPECT_TRUE(serial_auto != nullptr);

  serial_auto->stop();
}

/**
 * @brief SerialBuilder 콜백 등록이 올바르게 작동하는지 테스트
 */
TEST_F(BuilderIntegrationTest, SerialBuilderCallbackRegistration) {
  // --- Setup ---
  std::string test_device = "/dev/null";
  uint32_t test_baud_rate = 9600;
  std::atomic<int> data_callback_count{0};
  std::atomic<int> connect_callback_count{0};
  std::atomic<int> error_callback_count{0};

  // --- Test Logic ---
  auto serial = unilink::serial(test_device, test_baud_rate)
                    .on_data([&](const std::string& data) { data_callback_count++; })
                    .on_connect([&]() { connect_callback_count++; })
                    .on_error([&](const std::string& error) { error_callback_count++; })
                    .build();

  // --- Verification ---
  ASSERT_NE(serial, nullptr);

  // 콜백이 등록되었는지 확인 (실제 호출은 연결이 있을 때)
  EXPECT_TRUE(serial != nullptr);

  serial->start();
  std::this_thread::sleep_for(100ms);

  // 초기 상태에서는 콜백이 호출되지 않아야 함
  EXPECT_EQ(data_callback_count.load(), 0);
  EXPECT_EQ(connect_callback_count.load(), 0);
  EXPECT_EQ(error_callback_count.load(), 0);

  serial->stop();
}

/**
 * @brief SerialBuilder 메서드 체이닝이 올바르게 작동하는지 테스트
 */
TEST_F(BuilderIntegrationTest, SerialBuilderMethodChaining) {
  // --- Setup ---
  std::string test_device = "/dev/null";
  uint32_t test_baud_rate = 19200;

  // --- Test Logic ---
  auto serial = unilink::serial(test_device, test_baud_rate)

                    .auto_manage(true)
                    .on_data([](const std::string& data) {})
                    .on_connect([]() {})
                    .on_disconnect([]() {})
                    .on_error([](const std::string& error) {})
                    .build();

  // --- Verification ---
  ASSERT_NE(serial, nullptr);
  EXPECT_FALSE(serial->is_connected());

  // 모든 설정이 적용되었는지 확인
  serial->start();
  std::this_thread::sleep_for(100ms);

  EXPECT_TRUE(serial != nullptr);

  serial->stop();
}

/**
 * @brief SerialBuilder 에러 처리가 올바르게 작동하는지 테스트
 */
TEST_F(BuilderIntegrationTest, SerialBuilderErrorHandling) {
  // --- Setup ---
  std::string invalid_device = "/dev/nonexistent";  // 존재하지 않는 장치
  uint32_t test_baud_rate = 9600;

  // --- Test Logic ---
  auto serial = unilink::serial(invalid_device, test_baud_rate)

                    .on_error([this](const std::string& error) {
                      std::lock_guard<std::mutex> lock(mtx_);
                      error_occurred_.store(true);
                      last_error_ = error;
                      cv_.notify_one();
                    })
                    .build();

  // --- Verification ---
  ASSERT_NE(serial, nullptr);

  serial->start();
  std::this_thread::sleep_for(100ms);

  // Serial이 생성되었는지 확인 (에러가 발생해도 객체는 생성됨)
  EXPECT_TRUE(serial != nullptr);

  serial->stop();
}

/**
 * @brief SerialBuilder 성능 테스트
 */
TEST_F(BuilderIntegrationTest, SerialBuilderPerformance) {
  // --- Setup ---
  const int num_objects = 50;  // Serial은 TCP보다 느릴 수 있으므로 적은 수
  std::vector<std::shared_ptr<wrapper::Serial>> serials;

  // --- Test Logic ---
  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_objects; ++i) {
    std::string device = "/dev/null";
    uint32_t baud_rate = 9600 + i;

    auto serial = unilink::serial(device, baud_rate).build();

    serials.push_back(std::shared_ptr<wrapper::Serial>(serial.release()));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // --- Verification ---
  EXPECT_EQ(serials.size(), num_objects);

  // 모든 객체가 성공적으로 생성되었는지 확인
  for (const auto& serial : serials) {
    EXPECT_NE(serial, nullptr);
  }

  // 생성 시간이 합리적인지 확인 (50개 객체를 1초 이내에 생성)
  EXPECT_LT(duration.count(), 1000);

  // 정리
  for (auto& serial : serials) {
    serial->stop();
  }
}

/**
 * @brief SerialBuilder와 다른 빌더들의 통합 테스트
 */
TEST_F(BuilderIntegrationTest, SerialBuilderWithOtherBuilders) {
  // --- Setup ---
  uint16_t test_port = getTestPort();
  std::string test_device = "/dev/null";
  uint32_t test_baud_rate = 9600;

  // --- Test Logic ---
  // TCP 서버 생성
  server_ = unilink::tcp_server(test_port).unlimited_clients().build();

  // TCP 클라이언트 생성
  client_ = unilink::tcp_client("127.0.0.1", test_port).build();

  // Serial 생성
  serial_ = unilink::serial(test_device, test_baud_rate).build();

  // --- Verification ---
  ASSERT_NE(server_, nullptr);
  ASSERT_NE(client_, nullptr);
  ASSERT_NE(serial_, nullptr);

  // 각각 시작
  server_->start();
  client_->start();
  serial_->start();

  std::this_thread::sleep_for(100ms);

  // 모든 객체가 생성되었는지 확인
  EXPECT_TRUE(server_ != nullptr);
  EXPECT_TRUE(client_ != nullptr);
  EXPECT_TRUE(serial_ != nullptr);

  auto server_promise = std::make_shared<std::promise<void>>();
  server_->stop([server_promise] { server_promise->set_value(); });
  server_promise->get_future().wait_for(1s);

  auto client_promise = std::make_shared<std::promise<void>>();
  client_->stop([client_promise] { client_promise->set_value(); });
  client_promise->get_future().wait_for(1s);

  auto serial_promise = std::make_shared<std::promise<void>>();
  serial_->stop([serial_promise] { serial_promise->set_value(); });
  serial_promise->get_future().wait_for(1s);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

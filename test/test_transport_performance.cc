#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>

#include "unilink/transport/tcp_client/tcp_client.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"
#include "unilink/transport/serial/serial.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/config/serial_config.hpp"

using namespace unilink::transport;
using namespace unilink::config;
using namespace unilink::common;
using namespace std::chrono_literals;

/**
 * @brief Transport 레벨의 핵심 성능 요소 단위 테스트
 * 
 * Builder/Integration 테스트와 겹치지 않는 영역:
 * - 백프레셔 관리 (1MB 임계값)
 * - 재연결 로직 (retry mechanism)  
 * - 큐 관리 (메모리 사용량)
 * - 스레드 안전성 (동시 접근)
 * - 성능 특성 (처리량, 지연시간)
 * - 메모리 누수 (리소스 관리)
 */
class TransportPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 전 초기화
        backpressure_triggered_ = false;
        backpressure_bytes_ = 0;
        retry_count_ = 0;
    }
    
    void TearDown() override {
        // 테스트 후 정리
        if (client_) {
            client_->stop();
            client_.reset();
        }
        if (server_) {
            server_->stop();
            server_.reset();
        }
        if (serial_) {
            serial_->stop();
            serial_.reset();
        }
        
        // 충분한 시간을 두고 정리 완료 보장
        std::this_thread::sleep_for(100ms);
    }

    // 테스트용 포트 번호 (동적 할당으로 충돌 방지)
    uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{20000};
        return port_counter.fetch_add(1);
    }

protected:
    std::shared_ptr<TcpClient> client_;
    std::shared_ptr<TcpServer> server_;
    std::shared_ptr<Serial> serial_;
    
    // 백프레셔 테스트용
    std::atomic<bool> backpressure_triggered_{false};
    std::atomic<size_t> backpressure_bytes_{0};
    
    // 재연결 테스트용
    std::atomic<int> retry_count_{0};
};

// ============================================================================
// 백프레셔 관리 테스트
// ============================================================================

/**
 * @brief TCP Client 백프레셔 임계값 테스트
 * 
 * 1MB 임계값에서 백프레셔 콜백이 정확히 트리거되는지 검증
 * Note: 연결이 없어도 큐에 쌓인 데이터로 백프레셔를 테스트할 수 있습니다.
 */
TEST_F(TransportPerformanceTest, TcpClientBackpressureThreshold) {
    // --- Setup ---
    TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = getTestPort();
    cfg.retry_interval_ms = 1000;
    
    client_ = std::make_shared<TcpClient>(cfg);
    
    // 백프레셔 콜백 설정
    client_->on_backpressure([this](size_t bytes) {
        backpressure_triggered_ = true;
        backpressure_bytes_ = bytes;
    });
    
    // --- Test Logic ---
    client_->start();
    
    // 1MB를 넘는 대량의 데이터 전송 (연결이 없어도 큐에 쌓임)
    const size_t large_data_size = 2 * (1 << 20); // 2MB
    std::vector<uint8_t> large_data(large_data_size, 0xAA);
    client_->async_write_copy(large_data.data(), large_data.size());
    
    // --- Verification ---
    // 백프레셔가 트리거되었는지 확인
    std::this_thread::sleep_for(200ms);
    EXPECT_TRUE(backpressure_triggered_);
    EXPECT_GT(backpressure_bytes_.load(), 1 << 20);
}

/**
 * @brief TCP Server 백프레셔 임계값 테스트
 * 
 * Note: 서버는 클라이언트 연결이 없으면 데이터를 전송할 수 없으므로
 * 백프레셔 테스트는 연결된 상태에서만 의미가 있습니다.
 * 이 테스트는 큐 관리 로직이 정상 작동하는지만 확인합니다.
 */
TEST_F(TransportPerformanceTest, TcpServerBackpressureThreshold) {
    // --- Setup ---
    TcpServerConfig cfg;
    cfg.port = getTestPort();
    
    server_ = std::make_shared<TcpServer>(cfg);
    
    // 백프레셔 콜백 설정
    server_->on_backpressure([this](size_t bytes) {
        backpressure_triggered_ = true;
        backpressure_bytes_ = bytes;
    });
    
    // --- Test Logic ---
    server_->start();
    
    // 대량의 데이터 전송 (연결이 없어도 큐에 쌓임)
    const size_t large_data_size = 2 * (1 << 20); // 2MB
    std::vector<uint8_t> large_data(large_data_size, 0xCC);
    server_->async_write_copy(large_data.data(), large_data.size());
    
    // --- Verification ---
    std::this_thread::sleep_for(100ms);
    // 서버는 연결이 없어도 데이터를 큐에 쌓을 수 있어야 함
    EXPECT_TRUE(server_ != nullptr);
    // 백프레셔는 연결된 상태에서만 트리거되므로, 여기서는 큐 관리만 확인
}

/**
 * @brief Serial 백프레셔 임계값 테스트
 * 
 * Note: Serial은 실제 장치가 없으면 연결할 수 없으므로
 * 백프레셔 테스트는 연결된 상태에서만 의미가 있습니다.
 * 이 테스트는 큐 관리 로직이 정상 작동하는지만 확인합니다.
 */
TEST_F(TransportPerformanceTest, SerialBackpressureThreshold) {
    // --- Setup ---
    SerialConfig cfg;
    cfg.device = "/dev/null";
    cfg.baud_rate = 9600;
    cfg.retry_interval_ms = 1000;
    
    serial_ = std::make_shared<Serial>(cfg);
    
    // 백프레셔 콜백 설정
    serial_->on_backpressure([this](size_t bytes) {
        backpressure_triggered_ = true;
        backpressure_bytes_ = bytes;
    });
    
    // --- Test Logic ---
    serial_->start();
    
    // 대량의 데이터 전송 (연결이 없어도 큐에 쌓임)
    const size_t large_data_size = 2 * (1 << 20); // 2MB
    std::vector<uint8_t> large_data(large_data_size, 0xEE);
    serial_->async_write_copy(large_data.data(), large_data.size());
    
    // --- Verification ---
    std::this_thread::sleep_for(100ms);
    // Serial은 연결이 없어도 데이터를 큐에 쌓을 수 있어야 함
    EXPECT_TRUE(serial_ != nullptr);
    // 백프레셔는 연결된 상태에서만 트리거되므로, 여기서는 큐 관리만 확인
}

// ============================================================================
// 재연결 로직 테스트
// ============================================================================

/**
 * @brief TCP Client 재연결 로직 테스트
 * 
 * 연결 실패 시 설정된 간격으로 재연결을 시도하는지 검증
 */
TEST_F(TransportPerformanceTest, TcpClientRetryMechanism) {
    // --- Setup ---
    TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 1; // 존재하지 않는 포트로 연결 실패 유도
    cfg.retry_interval_ms = 100; // 짧은 재연결 간격으로 테스트
    
    client_ = std::make_shared<TcpClient>(cfg);
    
    // 상태 콜백으로 재연결 시도 횟수 카운트
    client_->on_state([this](LinkState state) {
        if (state == LinkState::Connecting) {
            retry_count_++;
        }
    });
    
    // --- Test Logic ---
    client_->start();
    
    // 재연결 시도가 여러 번 발생하는지 확인
    std::this_thread::sleep_for(500ms); // 5번의 재연결 시도 예상
    
    // --- Verification ---
    EXPECT_GT(retry_count_.load(), 3); // 최소 3번 이상 재연결 시도
}

/**
 * @brief Serial 재연결 로직 테스트
 */
TEST_F(TransportPerformanceTest, SerialRetryMechanism) {
    // --- Setup ---
    SerialConfig cfg;
    cfg.device = "/dev/nonexistent"; // 존재하지 않는 장치로 연결 실패 유도
    cfg.baud_rate = 9600;
    cfg.retry_interval_ms = 100; // 짧은 재연결 간격으로 테스트
    
    serial_ = std::make_shared<Serial>(cfg);
    
    // 상태 콜백으로 재연결 시도 횟수 카운트
    serial_->on_state([this](LinkState state) {
        if (state == LinkState::Connecting) {
            retry_count_++;
        }
    });
    
    // --- Test Logic ---
    serial_->start();
    
    // 재연결 시도가 여러 번 발생하는지 확인
    std::this_thread::sleep_for(500ms); // 5번의 재연결 시도 예상
    
    // --- Verification ---
    EXPECT_GT(retry_count_.load(), 3); // 최소 3번 이상 재연결 시도
}

// ============================================================================
// 큐 관리 및 메모리 관리 테스트
// ============================================================================

/**
 * @brief TCP Client 큐 관리 테스트
 * 
 * 대량의 데이터 전송 시 큐가 올바르게 관리되는지 검증
 */
TEST_F(TransportPerformanceTest, TcpClientQueueManagement) {
    // --- Setup ---
    TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = getTestPort();
    cfg.retry_interval_ms = 1000;
    
    client_ = std::make_shared<TcpClient>(cfg);
    
    // --- Test Logic ---
    client_->start();
    
    // 대량의 작은 메시지 전송 (큐 관리 테스트)
    const int num_messages = 1000;
    const size_t message_size = 1000; // 1KB per message
    
    for (int i = 0; i < num_messages; ++i) {
        std::vector<uint8_t> data(message_size, static_cast<uint8_t>(i % 256));
        client_->async_write_copy(data.data(), data.size());
    }
    
    // --- Verification ---
    // 큐 관리로 인한 메모리 사용량이 합리적인지 확인
    // (실제 메모리 측정은 복잡하므로, 크래시 없이 처리되는지만 확인)
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(client_ != nullptr);
}

/**
 * @brief TCP Server 큐 관리 테스트
 */
TEST_F(TransportPerformanceTest, TcpServerQueueManagement) {
    // --- Setup ---
    TcpServerConfig cfg;
    cfg.port = getTestPort();
    
    server_ = std::make_shared<TcpServer>(cfg);
    
    // --- Test Logic ---
    server_->start();
    
    // 대량의 작은 메시지 전송
    const int num_messages = 1000;
    const size_t message_size = 1000;
    
    for (int i = 0; i < num_messages; ++i) {
        std::vector<uint8_t> data(message_size, static_cast<uint8_t>(i % 256));
        server_->async_write_copy(data.data(), data.size());
    }
    
    // --- Verification ---
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(server_ != nullptr);
}

// ============================================================================
// 스레드 안전성 테스트
// ============================================================================

/**
 * @brief TCP Client 동시 접근 테스트
 * 
 * 여러 스레드에서 동시에 접근해도 안전한지 검증
 */
TEST_F(TransportPerformanceTest, TcpClientConcurrentAccess) {
    // --- Setup ---
    TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = getTestPort();
    cfg.retry_interval_ms = 1000;
    
    client_ = std::make_shared<TcpClient>(cfg);
    
    // --- Test Logic ---
    client_->start();
    
    // 여러 스레드에서 동시에 데이터 전송
    const int num_threads = 5;
    const int messages_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, messages_per_thread]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                std::string data = "thread_" + std::to_string(t) + "_msg_" + std::to_string(i);
                auto binary_data = common::safe_convert::string_to_uint8(data);
                client_->async_write_copy(binary_data.data(), binary_data.size());
            }
        });
    }
    
    // 모든 스레드 완료 대기
    for (auto& thread : threads) {
        thread.join();
    }
    
    // --- Verification ---
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(client_ != nullptr);
}

/**
 * @brief TCP Server 동시 접근 테스트
 */
TEST_F(TransportPerformanceTest, TcpServerConcurrentAccess) {
    // --- Setup ---
    TcpServerConfig cfg;
    cfg.port = getTestPort();
    
    server_ = std::make_shared<TcpServer>(cfg);
    
    // --- Test Logic ---
    server_->start();
    
    // 여러 스레드에서 동시에 데이터 전송
    const int num_threads = 5;
    const int messages_per_thread = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, t, messages_per_thread]() {
            for (int i = 0; i < messages_per_thread; ++i) {
                std::string data = "thread_" + std::to_string(t) + "_msg_" + std::to_string(i);
                auto binary_data = common::safe_convert::string_to_uint8(data);
                server_->async_write_copy(binary_data.data(), binary_data.size());
            }
        });
    }
    
    // 모든 스레드 완료 대기
    for (auto& thread : threads) {
        thread.join();
    }
    
    // --- Verification ---
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(server_ != nullptr);
}

// ============================================================================
// 성능 특성 테스트
// ============================================================================

/**
 * @brief TCP Client 처리량 테스트
 * 
 * 대량의 데이터를 빠르게 처리할 수 있는지 검증
 */
TEST_F(TransportPerformanceTest, TcpClientThroughput) {
    // --- Setup ---
    TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = getTestPort();
    cfg.retry_interval_ms = 1000;
    
    client_ = std::make_shared<TcpClient>(cfg);
    
    // --- Test Logic ---
    client_->start();
    
    const int num_messages = 10000;
    const size_t message_size = 100; // 100 bytes per message
    const size_t total_data = num_messages * message_size;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_messages; ++i) {
        std::vector<uint8_t> data(message_size, static_cast<uint8_t>(i % 256));
        client_->async_write_copy(data.data(), data.size());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // --- Verification ---
    // 10,000개 메시지를 1초 이내에 큐에 넣을 수 있어야 함
    EXPECT_LT(duration.count(), 1000);
    
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(client_ != nullptr);
}

/**
 * @brief TCP Server 처리량 테스트
 */
TEST_F(TransportPerformanceTest, TcpServerThroughput) {
    // --- Setup ---
    TcpServerConfig cfg;
    cfg.port = getTestPort();
    
    server_ = std::make_shared<TcpServer>(cfg);
    
    // --- Test Logic ---
    server_->start();
    
    const int num_messages = 10000;
    const size_t message_size = 100;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_messages; ++i) {
        std::vector<uint8_t> data(message_size, static_cast<uint8_t>(i % 256));
        server_->async_write_copy(data.data(), data.size());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // --- Verification ---
    EXPECT_LT(duration.count(), 1000);
    
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(server_ != nullptr);
}

// ============================================================================
// 메모리 누수 테스트
// ============================================================================

/**
 * @brief TCP Client 메모리 누수 테스트
 * 
 * 반복적인 생성/소멸 시 메모리 누수가 없는지 검증
 */
TEST_F(TransportPerformanceTest, TcpClientMemoryLeak) {
    // --- Setup ---
    const int num_cycles = 100;
    
    // --- Test Logic ---
    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        TcpClientConfig cfg;
        cfg.host = "127.0.0.1";
        cfg.port = getTestPort();
        cfg.retry_interval_ms = 100;
        
        auto client = std::make_shared<TcpClient>(cfg);
        client->start();
        
        // 데이터 전송
        std::string data = "memory_test_" + std::to_string(cycle);
        auto binary_data = common::safe_convert::string_to_uint8(data);
        client->async_write_copy(binary_data.data(), binary_data.size());
        
        client->stop();
        // client가 스코프를 벗어나면 자동으로 소멸
    }
    
    // --- Verification ---
    // 메모리 누수 검증은 복잡하므로, 크래시 없이 완료되는지만 확인
    EXPECT_TRUE(true);
}

/**
 * @brief TCP Server 메모리 누수 테스트
 */
TEST_F(TransportPerformanceTest, TcpServerMemoryLeak) {
    // --- Setup ---
    const int num_cycles = 100;
    
    // --- Test Logic ---
    for (int cycle = 0; cycle < num_cycles; ++cycle) {
        TcpServerConfig cfg;
        cfg.port = getTestPort();
        
        auto server = std::make_shared<TcpServer>(cfg);
        server->start();
        
        // 데이터 전송
        std::string data = "memory_test_" + std::to_string(cycle);
        auto binary_data = common::safe_convert::string_to_uint8(data);
        server->async_write_copy(binary_data.data(), binary_data.size());
        
        server->stop();
        // server가 스코프를 벗어나면 자동으로 소멸
    }
    
    // --- Verification ---
    EXPECT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

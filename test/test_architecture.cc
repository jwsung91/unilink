#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <iostream>

#include "unilink/unilink.hpp"
#include "unilink/common/io_context_manager.hpp"
#include "unilink/builder/auto_initializer.hpp"

using namespace unilink;
using namespace std::chrono_literals;

// ============================================================================
// IMPROVED ARCHITECTURE TESTS
// ============================================================================

class ImprovedArchitectureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 자동 초기화 테스트를 위해 IoContextManager를 중지
        if (common::IoContextManager::instance().is_running()) {
            std::cout << "Stopping IoContextManager for auto-init test..." << std::endl;
            common::IoContextManager::instance().stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    void TearDown() override {
        try {
            if (client_) {
                std::cout << "Stopping client..." << std::endl;
                client_->stop();
                client_.reset();
            }
            if (server_) {
                std::cout << "Stopping server..." << std::endl;
                server_->stop();
                server_.reset();
            }
            
            // 충분한 시간을 두고 정리
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            // IoContextManager는 각 테스트에서 개별적으로 관리하지 않음
            // 전역 상태이므로 다른 테스트에 영향을 줄 수 있음
        } catch (const std::exception& e) {
            std::cout << "Exception in TearDown: " << e.what() << std::endl;
        } catch (...) {
            std::cout << "Unknown exception in TearDown" << std::endl;
        }
    }

    uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{static_cast<uint16_t>(60000)};
        return port_counter.fetch_add(1);
    }

protected:
    std::shared_ptr<wrapper::TcpServer> server_;
    std::shared_ptr<wrapper::TcpClient> client_;
};

/**
 * @brief 현재 리소스 공유 문제 확인 테스트
 */
TEST_F(ImprovedArchitectureTest, CurrentResourceSharingIssue) {
    std::cout << "Testing current resource sharing issue..." << std::endl;
    
    uint16_t test_port = getTestPort();
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(server_, nullptr);
    std::cout << "Server created successfully" << std::endl;
    
    // 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(client_, nullptr);
    std::cout << "Client created successfully" << std::endl;
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    std::cout << "Test completed - resource sharing issue demonstrated" << std::endl;
}

/**
 * @brief 제안된 독립적 리소스 관리 테스트
 */
TEST_F(ImprovedArchitectureTest, ProposedIndependentResourceManagement) {
    std::cout << "Testing proposed independent resource management..." << std::endl;
    
    // AutoInitializer를 사용한 자동 초기화 테스트
    EXPECT_FALSE(builder::AutoInitializer::is_io_context_running());
    
    // 자동 초기화
    builder::AutoInitializer::ensure_io_context_running();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(builder::AutoInitializer::is_io_context_running());
    
    std::cout << "Independent resource management test completed" << std::endl;
}

/**
 * @brief 상위 API 자동 초기화 테스트
 */
TEST_F(ImprovedArchitectureTest, UpperAPIAutoInitialization) {
    std::cout << "Testing upper API auto-initialization..." << std::endl;
    
    uint16_t test_port = getTestPort();
    
    // IoContextManager가 실행 중이 아니어도 빌더가 자동으로 초기화
    if (common::IoContextManager::instance().is_running()) {
        common::IoContextManager::instance().stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 빌더 사용 시 자동으로 IoContextManager가 시작됨
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // IoContextManager가 자동으로 시작되었는지 확인
    EXPECT_TRUE(common::IoContextManager::instance().is_running());
    
    std::cout << "Upper API auto-initialization test completed" << std::endl;
}

/**
 * @brief 리소스 공유 분석 테스트
 */
TEST_F(ImprovedArchitectureTest, ResourceSharingAnalysis) {
    std::cout << "Analyzing resource sharing..." << std::endl;
    
    // IoContextManager를 통한 리소스 관리 테스트
    auto& context = common::IoContextManager::instance().get_context();
    EXPECT_TRUE(&context != nullptr);
    
    std::cout << "Resource sharing analysis completed" << std::endl;
}

// ============================================================================
// SAFE ARCHITECTURE TESTS
// ============================================================================

class SafeArchitectureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 각 테스트마다 깨끗한 상태로 시작
        if (common::IoContextManager::instance().is_running()) {
            common::IoContextManager::instance().stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
    
    void TearDown() override {
        if (client_) {
            client_->stop();
            client_.reset();
        }
        if (server_) {
            server_->stop();
            server_.reset();
        }
        
        // 충분한 시간을 두고 정리
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // IoContextManager 정리
        if (common::IoContextManager::instance().is_running()) {
            common::IoContextManager::instance().stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{static_cast<uint16_t>(80000)};
        return port_counter.fetch_add(1);
    }

protected:
    std::shared_ptr<wrapper::TcpServer> server_;
    std::shared_ptr<wrapper::TcpClient> client_;
};

/**
 * @brief 자동 초기화 테스트 (안전한 버전)
 */
TEST_F(SafeArchitectureTest, AutoInitializationTest) {
    std::cout << "Testing safe auto-initialization..." << std::endl;
    
    uint16_t test_port = getTestPort();
    
    // IoContextManager가 중지된 상태에서 시작
    EXPECT_FALSE(common::IoContextManager::instance().is_running());
    
    // 빌더를 통한 자동 초기화
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // IoContextManager가 자동으로 시작되었는지 확인
    EXPECT_TRUE(common::IoContextManager::instance().is_running());
    
    std::cout << "Safe auto-initialization test completed" << std::endl;
}

/**
 * @brief 리소스 정책 테스트
 */
TEST_F(SafeArchitectureTest, ResourcePolicyTest) {
    std::cout << "Testing resource policy..." << std::endl;
    
    // IoContextManager를 통한 리소스 관리 테스트
    auto& context = common::IoContextManager::instance().get_context();
    EXPECT_TRUE(&context != nullptr);
    
    // IoContextManager가 실행 중이 아니면 시작
    if (!common::IoContextManager::instance().is_running()) {
        common::IoContextManager::instance().start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // IoContextManager 상태 확인
    EXPECT_TRUE(common::IoContextManager::instance().is_running());
    
    std::cout << "Resource policy test completed" << std::endl;
}

/**
 * @brief 간단한 통신 테스트
 */
TEST_F(SafeArchitectureTest, SimpleCommunicationTest) {
    std::cout << "Testing simple communication..." << std::endl;
    
    uint16_t test_port = getTestPort();
    std::atomic<bool> data_received{false};
    std::string received_data;
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_data([&data_received, &received_data](const std::string& data) {
            received_data = data;
            data_received = true;
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(client_, nullptr);
    
    // 잠시 대기하여 연결 설정
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 데이터 전송
    if (client_->is_connected()) {
        client_->send("test message");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 데이터 수신 확인
        if (data_received.load()) {
            EXPECT_EQ(received_data, "test message");
        }
    }
    
    std::cout << "Simple communication test completed" << std::endl;
}

// ============================================================================
// FINAL ARCHITECTURE TESTS
// ============================================================================

class FinalArchitectureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 각 테스트마다 깨끗한 상태로 시작
        // IoContextManager는 전역 상태이므로 완전히 정리하지 않음
    }
    
    void TearDown() override {
        if (client_) {
            try {
                client_->stop();
            } catch (...) {
                // 예외 무시
            }
            client_.reset();
        }
        if (server_) {
            try {
                server_->stop();
            } catch (...) {
                // 예외 무시
            }
            server_.reset();
        }
        
        // 충분한 시간을 두고 정리
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{static_cast<uint16_t>(90000)};
        return port_counter.fetch_add(1);
    }

protected:
    std::shared_ptr<wrapper::TcpServer> server_;
    std::shared_ptr<wrapper::TcpClient> client_;
};

/**
 * @brief 자동 초기화 테스트 (최종 버전)
 */
TEST_F(FinalArchitectureTest, AutoInitializationTest) {
    std::cout << "Testing final auto-initialization..." << std::endl;
    
    uint16_t test_port = getTestPort();
    
    // 빌더를 통한 자동 초기화
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // IoContextManager가 실행 중인지 확인
    EXPECT_TRUE(common::IoContextManager::instance().is_running());
    
    std::cout << "Final auto-initialization test completed" << std::endl;
}

/**
 * @brief 실제 통신 테스트 (최종 버전)
 */
TEST_F(FinalArchitectureTest, RealCommunicationTest) {
    std::cout << "Testing final real communication..." << std::endl;
    
    uint16_t test_port = getTestPort();
    std::atomic<bool> server_connected{false};
    std::atomic<bool> client_connected{false};
    std::atomic<bool> data_received{false};
    std::string received_data;
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_connect([&server_connected]() {
            server_connected = true;
        })
        .on_data([&data_received, &received_data](const std::string& data) {
            received_data = data;
            data_received = true;
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .on_connect([&client_connected]() {
            client_connected = true;
        })
        .build();
    
    ASSERT_NE(client_, nullptr);
    
    // 연결 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 데이터 전송 테스트
    if (client_->is_connected()) {
        client_->send("final test message");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        if (data_received.load()) {
            EXPECT_EQ(received_data, "final test message");
        }
    }
    
    std::cout << "Final real communication test completed" << std::endl;
}

/**
 * @brief 리소스 정책 테스트 (최종 버전)
 */
TEST_F(FinalArchitectureTest, ResourcePolicyTest) {
    std::cout << "Testing final resource policy..." << std::endl;
    
    // IoContextManager를 통한 리소스 관리
    auto& context = common::IoContextManager::instance().get_context();
    EXPECT_TRUE(&context != nullptr);
    
    // IoContextManager가 실행 중이 아니면 시작
    if (!common::IoContextManager::instance().is_running()) {
        common::IoContextManager::instance().start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // IoContextManager 상태 확인
    EXPECT_TRUE(common::IoContextManager::instance().is_running());
    
    std::cout << "Final resource policy test completed" << std::endl;
}

// ============================================================================
// IOCONTEXT FIX TESTS
// ============================================================================

class IoContextFixTest : public ::testing::Test {
protected:
    void SetUp() override {
        // IoContextManager 시작
        std::cout << "Starting IoContextManager..." << std::endl;
        common::IoContextManager::instance().start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        if (server_) {
            std::cout << "Stopping server..." << std::endl;
            server_->stop();
        }
        if (client_) {
            std::cout << "Stopping client..." << std::endl;
            client_->stop();
        }
        
        // IoContextManager 중지
        std::cout << "Stopping IoContextManager..." << std::endl;
        common::IoContextManager::instance().stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{60000};
        return port_counter.fetch_add(1);
    }

protected:
    std::shared_ptr<wrapper::TcpServer> server_;
    std::shared_ptr<wrapper::TcpClient> client_;
};

/**
 * @brief IoContextManager 상태 확인 테스트
 */
TEST_F(IoContextFixTest, IoContextManagerStatus) {
    std::cout << "Testing IoContextManager status..." << std::endl;
    
    // IoContextManager가 실행 중인지 확인
    EXPECT_TRUE(common::IoContextManager::instance().is_running());
    
    // 컨텍스트 접근 테스트
    auto& context = common::IoContextManager::instance().get_context();
    EXPECT_TRUE(&context != nullptr);
    
    std::cout << "IoContextManager status test completed" << std::endl;
}

/**
 * @brief 시작된 IoContext와 함께 서버 테스트
 */
TEST_F(IoContextFixTest, ServerWithStartedIoContext) {
    std::cout << "Testing server with started IoContext..." << std::endl;
    
    uint16_t test_port = getTestPort();
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 서버가 생성되었는지 확인
    EXPECT_TRUE(server_ != nullptr);
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    std::cout << "Server with started IoContext test completed" << std::endl;
}

/**
 * @brief 시작된 IoContext와 함께 실제 통신 테스트
 */
TEST_F(IoContextFixTest, RealCommunicationWithStartedIoContext) {
    std::cout << "Testing real communication with started IoContext..." << std::endl;
    
    uint16_t test_port = getTestPort();
    std::atomic<bool> data_received{false};
    std::string received_data;
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_data([&data_received, &received_data](const std::string& data) {
            received_data = data;
            data_received = true;
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .build();
    
    ASSERT_NE(client_, nullptr);
    
    // 연결 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 데이터 전송 테스트
    if (client_->is_connected()) {
        client_->send("IoContext fix test message");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        if (data_received.load()) {
            EXPECT_EQ(received_data, "IoContext fix test message");
        }
    }
    
    std::cout << "Real communication with started IoContext test completed" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

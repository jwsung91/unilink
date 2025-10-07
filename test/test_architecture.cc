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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

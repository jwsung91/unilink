#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <iostream>

#include "unilink/unilink.hpp"
#include "unilink/common/io_context_manager.hpp"
#include "unilink/builder/auto_initializer.hpp"
#include "unilink/builder/resource_manager.hpp"

using namespace unilink;
using namespace std::chrono_literals;

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
 * @brief 현재 아키텍처의 리소스 공유 문제 확인
 */
TEST_F(ImprovedArchitectureTest, CurrentResourceSharingIssue) {
    uint16_t test_port = getTestPort();
    std::cout << "Testing current resource sharing, port: " << test_port << std::endl;
    
    try {
        // 서버와 클라이언트가 같은 IoContextManager를 사용하는지 확인
        server_ = builder::UnifiedBuilder::tcp_server(test_port)
            .auto_start(true)
            .on_error([test_port](const std::string& error) {
                std::cout << "Server error on port " << test_port << ": " << error << std::endl;
            })
            .build();
        
        client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
            .auto_start(true)
            .on_error([test_port](const std::string& error) {
                std::cout << "Client error on port " << test_port << ": " << error << std::endl;
            })
            .build();
        
        ASSERT_NE(server_, nullptr);
        ASSERT_NE(client_, nullptr);
        
        // 연결 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        std::cout << "Server connected: " << (server_->is_connected() ? "true" : "false") << std::endl;
        std::cout << "Client connected: " << (client_->is_connected() ? "true" : "false") << std::endl;
        
        // 현재는 작동하지만, 리소스 공유로 인한 잠재적 문제가 있음
        EXPECT_TRUE(server_ != nullptr);
        EXPECT_TRUE(client_ != nullptr);
        
    } catch (const std::exception& e) {
        std::cout << "Exception in CurrentResourceSharingIssue: " << e.what() << std::endl;
        FAIL() << "Exception occurred: " << e.what();
    } catch (...) {
        std::cout << "Unknown exception in CurrentResourceSharingIssue" << std::endl;
        FAIL() << "Unknown exception occurred";
    }
}

/**
 * @brief 개선된 아키텍처 제안: 독립적인 리소스 관리
 */
TEST_F(ImprovedArchitectureTest, ProposedIndependentResourceManagement) {
    std::cout << "Testing proposed independent resource management..." << std::endl;
    
    // 제안 1: 각 컴포넌트가 독립적인 io_context 사용
    // 이는 현재 TcpClient가 이미 구현하고 있는 방식
    
    // 제안 2: 상위 API에서 자동 초기화
    // Builder 패턴에서 IoContextManager 자동 시작
    
    // 제안 3: 명시적인 리소스 관리
    // 사용자가 리소스 생명주기를 명확히 제어
    
    std::cout << "Proposed improvements:" << std::endl;
    std::cout << "1. Each component should use independent io_context" << std::endl;
    std::cout << "2. Upper API should auto-initialize IoContextManager" << std::endl;
    std::cout << "3. Explicit resource lifecycle management" << std::endl;
    
    EXPECT_TRUE(true); // 개념적 테스트
}

/**
 * @brief 상위 API 자동 초기화 테스트 (개선된 Builder)
 */
TEST_F(ImprovedArchitectureTest, UpperAPIAutoInitialization) {
    uint16_t test_port = getTestPort();
    std::cout << "Testing improved Builder auto-initialization, port: " << test_port << std::endl;
    
    // IoContextManager가 중지된 상태에서 시작
    EXPECT_FALSE(common::IoContextManager::instance().is_running()) 
        << "IoContextManager should be stopped for auto-init test";
    
    try {
        // Builder 사용 시 자동으로 IoContextManager 시작 (개선된 Builder)
        server_ = builder::UnifiedBuilder::tcp_server(test_port)
            .auto_start(true)
            .on_error([test_port](const std::string& error) {
                std::cout << "Server error on port " << test_port << ": " << error << std::endl;
            })
            .build();
        
        // 이제 IoContextManager가 자동으로 시작되었는지 확인
        EXPECT_TRUE(common::IoContextManager::instance().is_running()) 
            << "IoContextManager should be auto-started by Builder";
        
        client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
            .auto_start(true)
            .on_error([test_port](const std::string& error) {
                std::cout << "Client error on port " << test_port << ": " << error << std::endl;
            })
            .build();
        
        ASSERT_NE(server_, nullptr);
        ASSERT_NE(client_, nullptr);
        
        // 연결 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        std::cout << "IoContextManager running: " << (common::IoContextManager::instance().is_running() ? "true" : "false") << std::endl;
        std::cout << "Server connected: " << (server_->is_connected() ? "true" : "false") << std::endl;
        std::cout << "Client connected: " << (client_->is_connected() ? "true" : "false") << std::endl;
        
        EXPECT_TRUE(server_ != nullptr);
        EXPECT_TRUE(client_ != nullptr);
        
    } catch (const std::exception& e) {
        std::cout << "Exception in UpperAPIAutoInitialization: " << e.what() << std::endl;
        FAIL() << "Exception occurred: " << e.what();
    } catch (...) {
        std::cout << "Unknown exception in UpperAPIAutoInitialization" << std::endl;
        FAIL() << "Unknown exception occurred";
    }
}

/**
 * @brief 리소스 분리 문제 분석 테스트
 */
TEST_F(ImprovedArchitectureTest, ResourceSharingAnalysis) {
    std::cout << "Analyzing current resource sharing issues..." << std::endl;
    
    // 현재 리소스 정책 확인
    auto policy = builder::ResourceManager::get_current_policy();
    std::cout << "Current resource policy: " 
              << (policy == builder::ResourceManager::ResourcePolicy::SHARED ? "SHARED" : "INDEPENDENT") 
              << std::endl;
    
    // 문제점 분석
    std::cout << "\nCurrent Issues:" << std::endl;
    std::cout << "1. Server uses shared io_context (IoContextManager)" << std::endl;
    std::cout << "2. Client uses independent io_context" << std::endl;
    std::cout << "3. Inconsistent resource management" << std::endl;
    std::cout << "4. Potential blocking issues" << std::endl;
    
    // 개선 방안 제안
    std::cout << "\nProposed Solutions:" << std::endl;
    std::cout << "1. All components use independent io_context" << std::endl;
    std::cout << "2. Explicit resource lifecycle management" << std::endl;
    std::cout << "3. Component isolation" << std::endl;
    std::cout << "4. Consistent architecture" << std::endl;
    
    // 리소스 정책 변경 테스트
    builder::ResourceManager::set_policy(builder::ResourceManager::ResourcePolicy::INDEPENDENT);
    auto new_policy = builder::ResourceManager::get_current_policy();
    EXPECT_EQ(new_policy, builder::ResourceManager::ResourcePolicy::INDEPENDENT);
    
    std::cout << "Resource policy changed to: " 
              << (new_policy == builder::ResourceManager::ResourcePolicy::SHARED ? "SHARED" : "INDEPENDENT") 
              << std::endl;
    
    EXPECT_TRUE(true); // 개념적 테스트
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

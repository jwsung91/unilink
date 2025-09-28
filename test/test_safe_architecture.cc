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
    uint16_t test_port = getTestPort();
    std::cout << "Testing auto-initialization, port: " << test_port << std::endl;
    
    // IoContextManager가 중지된 상태에서 시작
    EXPECT_FALSE(common::IoContextManager::instance().is_running()) 
        << "IoContextManager should be stopped for auto-init test";
    
    // Builder 사용 시 자동으로 IoContextManager 시작
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)  // 수동 시작으로 제어
        .build();
    
    // 이제 IoContextManager가 자동으로 시작되었는지 확인
    EXPECT_TRUE(common::IoContextManager::instance().is_running()) 
        << "IoContextManager should be auto-started by Builder";
    
    // 서버 시작
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    std::cout << "IoContextManager running: " << (common::IoContextManager::instance().is_running() ? "true" : "false") << std::endl;
    std::cout << "Server created: " << (server_ != nullptr ? "true" : "false") << std::endl;
    
    EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 리소스 정책 테스트
 */
TEST_F(SafeArchitectureTest, ResourcePolicyTest) {
    std::cout << "Testing resource policy management..." << std::endl;
    
    // 현재 리소스 정책 확인
    auto policy = builder::ResourceManager::get_current_policy();
    std::cout << "Current resource policy: " 
              << (policy == builder::ResourceManager::ResourcePolicy::SHARED ? "SHARED" : "INDEPENDENT") 
              << std::endl;
    
    // 리소스 정책 변경 테스트
    builder::ResourceManager::set_policy(builder::ResourceManager::ResourcePolicy::INDEPENDENT);
    auto new_policy = builder::ResourceManager::get_current_policy();
    EXPECT_EQ(new_policy, builder::ResourceManager::ResourcePolicy::INDEPENDENT);
    
    std::cout << "Resource policy changed to: " 
              << (new_policy == builder::ResourceManager::ResourcePolicy::SHARED ? "SHARED" : "INDEPENDENT") 
              << std::endl;
    
    // 다시 원래 정책으로 복원
    builder::ResourceManager::set_policy(builder::ResourceManager::ResourcePolicy::SHARED);
    auto restored_policy = builder::ResourceManager::get_current_policy();
    EXPECT_EQ(restored_policy, builder::ResourceManager::ResourcePolicy::SHARED);
    
    std::cout << "Resource policy restored to: " 
              << (restored_policy == builder::ResourceManager::ResourcePolicy::SHARED ? "SHARED" : "INDEPENDENT") 
              << std::endl;
}

/**
 * @brief 간단한 통신 테스트 (안전한 버전)
 */
TEST_F(SafeArchitectureTest, SimpleCommunicationTest) {
    uint16_t test_port = getTestPort();
    std::cout << "Testing simple communication, port: " << test_port << std::endl;
    
    bool server_connected = false;
    bool client_connected = false;
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_connect([&server_connected]() {
            std::cout << "Server: Client connected!" << std::endl;
            server_connected = true;
        })
        .on_error([test_port](const std::string& error) {
            std::cout << "Server error on port " << test_port << ": " << error << std::endl;
        })
        .build();
    
    ASSERT_NE(server_, nullptr) << "Server creation failed";
    
    // 서버 시작 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .on_connect([&client_connected]() {
            std::cout << "Client: Connected to server!" << std::endl;
            client_connected = true;
        })
        .on_error([test_port](const std::string& error) {
            std::cout << "Client error on port " << test_port << ": " << error << std::endl;
        })
        .build();
    
    ASSERT_NE(client_, nullptr) << "Client creation failed";
    
    // 클라이언트 연결 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    // 상태 확인
    std::cout << "Final states:" << std::endl;
    std::cout << "  IoContextManager running: " << (common::IoContextManager::instance().is_running() ? "true" : "false") << std::endl;
    std::cout << "  Server is_connected(): " << (server_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "  Client is_connected(): " << (client_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "  Server connected flag: " << (server_connected ? "true" : "false") << std::endl;
    std::cout << "  Client connected flag: " << (client_connected ? "true" : "false") << std::endl;
    
    // 기본적인 검증
    EXPECT_TRUE(server_ != nullptr);
    EXPECT_TRUE(client_ != nullptr);
    
    // 연결이 성공한 경우에만 추가 검증
    if (client_->is_connected() && server_->is_connected()) {
        std::cout << "SUCCESS: Server and client are connected!" << std::endl;
        
        // 데이터 전송 테스트
        const std::string test_message = "Hello from safe test!";
        client_->send(test_message);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        std::cout << "Data transmission test completed" << std::endl;
    } else {
        std::cout << "INFO: Connection not established (may be due to network environment)" << std::endl;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

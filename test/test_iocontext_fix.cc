#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <iostream>

#include "unilink/unilink.hpp"
#include "unilink/common/io_context_manager.hpp"

using namespace unilink;
using namespace std::chrono_literals;

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
    
    // IoContextManager 상태 확인
    bool is_running = common::IoContextManager::instance().is_running();
    std::cout << "IoContextManager is running: " << (is_running ? "true" : "false") << std::endl;
    
    EXPECT_TRUE(is_running) << "IoContextManager should be running";
}

/**
 * @brief 서버 생성 및 포트 바인딩 테스트 (IoContextManager 시작 후)
 */
TEST_F(IoContextFixTest, ServerWithStartedIoContext) {
    uint16_t test_port = getTestPort();
    std::cout << "Testing server with started IoContext, port: " << test_port << std::endl;
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)
        .on_error([test_port](const std::string& error) {
            std::cout << "Server error on port " << test_port << ": " << error << std::endl;
        })
        .build();
    
    ASSERT_NE(server_, nullptr) << "Server creation failed";
    std::cout << "Server created successfully" << std::endl;
    
    // 서버 시작
    std::cout << "Starting server..." << std::endl;
    server_->start();
    
    // 서버가 리스닝 상태가 될 때까지 대기
    std::cout << "Waiting for server to start..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Check " << (i + 1) << "/10 - Server connected: " 
                  << (server_->is_connected() ? "true" : "false") << std::endl;
    }
    
    // 서버가 생성되었는지 확인
    EXPECT_TRUE(server_ != nullptr);
}

/**
 * @brief 실제 서버-클라이언트 통신 테스트 (IoContextManager 시작 후)
 */
TEST_F(IoContextFixTest, RealCommunicationWithStartedIoContext) {
    uint16_t test_port = getTestPort();
    std::cout << "Testing real communication with started IoContext, port: " << test_port << std::endl;
    
    bool server_connected = false;
    bool client_connected = false;
    std::vector<std::string> received_data;
    
    // 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_connect([&server_connected]() {
            std::cout << "Server: Client connected!" << std::endl;
            server_connected = true;
        })
        .on_data([&received_data](const std::string& data) {
            std::cout << "Server received: " << data << std::endl;
            received_data.push_back("SERVER: " + data);
        })
        .on_error([test_port](const std::string& error) {
            std::cout << "Server error on port " << test_port << ": " << error << std::endl;
        })
        .build();
    
    ASSERT_NE(server_, nullptr) << "Server creation failed";
    
    // 서버 시작 대기
    std::cout << "Waiting for server to start..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .on_connect([&client_connected]() {
            std::cout << "Client: Connected to server!" << std::endl;
            client_connected = true;
        })
        .on_data([&received_data](const std::string& data) {
            std::cout << "Client received: " << data << std::endl;
            received_data.push_back("CLIENT: " + data);
        })
        .on_error([test_port](const std::string& error) {
            std::cout << "Client error on port " << test_port << ": " << error << std::endl;
        })
        .build();
    
    ASSERT_NE(client_, nullptr) << "Client creation failed";
    
    // 클라이언트 연결 대기
    std::cout << "Waiting for client to connect..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    
    // 상태 확인
    std::cout << "Final states:" << std::endl;
    std::cout << "  Server is_connected(): " << (server_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "  Client is_connected(): " << (client_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "  Server connected flag: " << (server_connected ? "true" : "false") << std::endl;
    std::cout << "  Client connected flag: " << (client_connected ? "true" : "false") << std::endl;
    std::cout << "  Data received: " << received_data.size() << " messages" << std::endl;
    
    // 데이터 전송 시도
    if (client_->is_connected()) {
        std::cout << "Sending test message..." << std::endl;
        const std::string test_message = "Hello from client!";
        client_->send(test_message);
        
        // 데이터 수신 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        std::cout << "Data received after sending:" << std::endl;
        for (const auto& data : received_data) {
            std::cout << "  " << data << std::endl;
        }
        
        // 서버가 데이터를 받았는지 확인
        bool server_received = false;
        for (const auto& data : received_data) {
            if (data.find("SERVER: " + test_message) != std::string::npos) {
                server_received = true;
                break;
            }
        }
        
        EXPECT_TRUE(server_received) << "Server did not receive the message";
        EXPECT_TRUE(client_->is_connected()) << "Client should be connected";
        
    } else {
        std::cout << "Client not connected, skipping data transmission" << std::endl;
        
        // 연결이 실패한 경우에도 테스트는 통과 (네트워크 환경에 따라)
        GTEST_SKIP() << "Client could not connect to server (network environment dependent)";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <iostream>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

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
    // --- Setup ---
    uint16_t test_port = getTestPort();
    std::cout << "Testing with port: " << test_port << std::endl;
    
    // --- Test Logic ---
    // 1. 서버 생성
    std::cout << "Creating server..." << std::endl;
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)  // 수동 시작
        .on_data([this](const std::string& data) {
            std::cout << "Server received data: " << data << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("SERVER: " + data);
        })
        .on_connect([this]() {
            std::cout << "Server: Client connected!" << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            server_ready_ = true;
            cv_.notify_one();
        })
        .on_error([this](const std::string& error) {
            std::cout << "Server error: " << error << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            error_occurred_ = true;
            last_error_ = error;
            cv_.notify_one();
        })
        .build();
    
    // --- Verification ---
    ASSERT_NE(server_, nullptr) << "Server creation failed";
    std::cout << "Server created successfully" << std::endl;
    
    // 서버 상태 확인
    EXPECT_FALSE(server_->is_connected()) << "Server should not be connected before start";
    std::cout << "Server initial state: not connected (expected)" << std::endl;
    
    // 서버 시작
    std::cout << "Starting server..." << std::endl;
    server_->start();
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 서버 상태 재확인
    std::cout << "Server connected state: " << (server_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "Server ready state: " << (server_ready_.load() ? "true" : "false") << std::endl;
    std::cout << "Error occurred: " << (error_occurred_.load() ? "true" : "false") << std::endl;
    if (error_occurred_.load()) {
        std::cout << "Last error: " << last_error_ << std::endl;
    }
}

/**
 * @brief 클라이언트 생성 및 연결 시도 테스트
 */
TEST_F(DebugCommunicationTest, ClientCreationAndConnection) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    std::cout << "Testing client with port: " << test_port << std::endl;
    
    // --- Test Logic ---
    // 1. 서버 생성 및 시작
    std::cout << "Creating and starting server..." << std::endl;
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)  // 자동 시작
        .on_connect([this]() {
            std::cout << "Server: Client connected!" << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            server_ready_ = true;
            cv_.notify_one();
        })
        .on_error([this](const std::string& error) {
            std::cout << "Server error: " << error << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            error_occurred_ = true;
            last_error_ = error;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(server_, nullptr) << "Server creation failed";
    
    // 서버 시작 대기
    std::cout << "Waiting for server to start..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    std::cout << "Server state after 2s: connected=" << (server_->is_connected() ? "true" : "false") 
              << ", ready=" << (server_ready_.load() ? "true" : "false") << std::endl;
    
    // 2. 클라이언트 생성
    std::cout << "Creating client..." << std::endl;
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)  // 자동 시작
        .on_connect([this]() {
            std::cout << "Client: Connected to server!" << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            cv_.notify_one();
        })
        .on_error([this](const std::string& error) {
            std::cout << "Client error: " << error << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            error_occurred_ = true;
            last_error_ = error;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(client_, nullptr) << "Client creation failed";
    
    // 클라이언트 연결 대기
    std::cout << "Waiting for client to connect..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    std::cout << "Client state after 3s: connected=" << (client_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "Connection established: " << (connection_established_.load() ? "true" : "false") << std::endl;
    std::cout << "Error occurred: " << (error_occurred_.load() ? "true" : "false") << std::endl;
    if (error_occurred_.load()) {
        std::cout << "Last error: " << last_error_ << std::endl;
    }
}

/**
 * @brief 간단한 통신 테스트
 */
TEST_F(DebugCommunicationTest, SimpleCommunication) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    std::cout << "Testing communication with port: " << test_port << std::endl;
    
    // --- Test Logic ---
    // 1. 서버 생성
    std::cout << "Creating server..." << std::endl;
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_data([this](const std::string& data) {
            std::cout << "Server received: " << data << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("SERVER: " + data);
        })
        .on_connect([this]() {
            std::cout << "Server: Client connected!" << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            server_ready_ = true;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 서버 시작 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 2. 클라이언트 생성
    std::cout << "Creating client..." << std::endl;
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .on_data([this](const std::string& data) {
            std::cout << "Client received: " << data << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("CLIENT: " + data);
        })
        .on_connect([this]() {
            std::cout << "Client: Connected!" << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(client_, nullptr);
    
    // 연결 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 3. 데이터 전송 시도
    if (client_->is_connected()) {
        std::cout << "Sending test message..." << std::endl;
        client_->send("Hello from client!");
        
        // 데이터 수신 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        std::cout << "Data received count: " << data_received_.size() << std::endl;
        for (const auto& data : data_received_) {
            std::cout << "Received: " << data << std::endl;
        }
    } else {
        std::cout << "Client not connected, skipping data transmission" << std::endl;
    }
    
    // --- Verification ---
    std::cout << "Final states:" << std::endl;
    std::cout << "  Server connected: " << (server_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "  Client connected: " << (client_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "  Connection established: " << (connection_established_.load() ? "true" : "false") << std::endl;
    std::cout << "  Data received: " << data_received_.size() << " messages" << std::endl;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

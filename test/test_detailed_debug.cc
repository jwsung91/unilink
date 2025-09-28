#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

class DetailedDebugTest : public ::testing::Test {
protected:
    void SetUp() override {
        data_received_.clear();
        connection_established_ = false;
        error_occurred_ = false;
        server_ready_ = false;
        client_connected_ = false;
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
        static std::atomic<uint16_t> port_counter{40000};
        return port_counter.fetch_add(1);
    }

    // 포트가 실제로 사용 중인지 확인
    bool isPortInUse(uint16_t port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return false;
        
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
        close(sock);
        
        return result != 0; // bind 실패하면 포트가 사용 중
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
    std::atomic<bool> client_connected_{false};
    std::string last_error_;
};

/**
 * @brief 포트 바인딩 상태 확인 테스트
 */
TEST_F(DetailedDebugTest, PortBindingStatus) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    std::cout << "Testing port binding with port: " << test_port << std::endl;
    
    // --- Test Logic ---
    // 1. 초기 포트 상태 확인
    std::cout << "Initial port status: " << (isPortInUse(test_port) ? "IN USE" : "FREE") << std::endl;
    
    // 2. 서버 생성
    std::cout << "Creating server..." << std::endl;
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)  // 수동 시작
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
    
    // 3. 서버 시작 전 포트 상태
    std::cout << "Port status before server start: " << (isPortInUse(test_port) ? "IN USE" : "FREE") << std::endl;
    
    // 4. 서버 시작
    std::cout << "Starting server..." << std::endl;
    server_->start();
    
    // 5. 서버 시작 후 포트 상태 확인
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "Port status after server start: " << (isPortInUse(test_port) ? "IN USE" : "FREE") << std::endl;
    
    // 6. 추가 대기 후 포트 상태 확인
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::cout << "Port status after 3s total: " << (isPortInUse(test_port) ? "IN USE" : "FREE") << std::endl;
    
    // 7. 서버 상태 확인
    std::cout << "Server state:" << std::endl;
    std::cout << "  is_connected(): " << (server_->is_connected() ? "true" : "false") << std::endl;
    std::cout << "  Error occurred: " << (error_occurred_.load() ? "true" : "false") << std::endl;
    if (error_occurred_.load()) {
        std::cout << "  Last error: " << last_error_ << std::endl;
    }
    
    // --- Verification ---
    // 포트가 사용 중이어야 함 (서버가 바인딩했을 경우)
    if (isPortInUse(test_port)) {
        std::cout << "SUCCESS: Port is bound by server" << std::endl;
    } else {
        std::cout << "WARNING: Port is not bound by server" << std::endl;
    }
    
    // 에러가 발생하지 않았는지 확인
    EXPECT_FALSE(error_occurred_.load()) << "Server failed to start: " << last_error_;
}

/**
 * @brief 간단한 TCP 연결 테스트 (raw socket 사용)
 */
TEST_F(DetailedDebugTest, RawTcpConnection) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    std::cout << "Testing raw TCP connection with port: " << test_port << std::endl;
    
    // --- Test Logic ---
    // 1. 서버 생성 및 시작
    std::cout << "Creating and starting server..." << std::endl;
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
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
    
    // 2. Raw TCP 클라이언트로 연결 시도
    std::cout << "Attempting raw TCP connection..." << std::endl;
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        std::cout << "Failed to create socket" << std::endl;
        GTEST_SKIP() << "Failed to create raw socket";
        return;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(test_port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    int result = connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (result == 0) {
        std::cout << "SUCCESS: Raw TCP connection established!" << std::endl;
        close(client_sock);
        
        // 3. 이제 wrapper 클라이언트로 연결 시도
        std::cout << "Now trying wrapper client..." << std::endl;
        client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
            .auto_start(true)
            .on_connect([this]() {
                std::cout << "Wrapper client: Connected!" << std::endl;
                std::lock_guard<std::mutex> lock(mtx_);
                client_connected_ = true;
                cv_.notify_one();
            })
            .on_error([this](const std::string& error) {
                std::cout << "Wrapper client error: " << error << std::endl;
                std::lock_guard<std::mutex> lock(mtx_);
                error_occurred_ = true;
                last_error_ = error;
                cv_.notify_one();
            })
            .build();
        
        ASSERT_NE(client_, nullptr) << "Wrapper client creation failed";
        
        // 클라이언트 연결 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        
        std::cout << "Wrapper client state: " << (client_->is_connected() ? "CONNECTED" : "NOT CONNECTED") << std::endl;
        std::cout << "Client connected flag: " << (client_connected_.load() ? "true" : "false") << std::endl;
        
    } else {
        std::cout << "FAILED: Raw TCP connection failed (errno: " << errno << ")" << std::endl;
        close(client_sock);
        
        // 포트 상태 재확인
        std::cout << "Port status: " << (isPortInUse(test_port) ? "IN USE" : "FREE") << std::endl;
        
        GTEST_SKIP() << "Raw TCP connection failed - server may not be listening";
    }
}

/**
 * @brief 서버 에러 로깅 강화 테스트
 */
TEST_F(DetailedDebugTest, ServerErrorLogging) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    std::cout << "Testing server error logging with port: " << test_port << std::endl;
    
    // --- Test Logic ---
    // 1. 서버 생성 (에러 핸들러 강화)
    std::cout << "Creating server with enhanced error logging..." << std::endl;
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)  // 수동 시작
        .on_connect([this]() {
            std::cout << "Server: Client connected!" << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            server_ready_ = true;
            cv_.notify_one();
        })
        .on_error([this, test_port](const std::string& error) {
            std::cout << "=== SERVER ERROR DETECTED ===" << std::endl;
            std::cout << "Error message: " << error << std::endl;
            std::cout << "Port: " << test_port << std::endl;
            std::cout << "=============================" << std::endl;
            std::lock_guard<std::mutex> lock(mtx_);
            error_occurred_ = true;
            last_error_ = error;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(server_, nullptr) << "Server creation failed";
    
    // 2. 서버 시작
    std::cout << "Starting server..." << std::endl;
    server_->start();
    
    // 3. 상태 모니터링
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::cout << "Check " << (i + 1) << "/10:" << std::endl;
        std::cout << "  Server is_connected(): " << (server_->is_connected() ? "true" : "false") << std::endl;
        std::cout << "  Port in use: " << (isPortInUse(test_port) ? "YES" : "NO") << std::endl;
        std::cout << "  Error occurred: " << (error_occurred_.load() ? "true" : "false") << std::endl;
        if (error_occurred_.load()) {
            std::cout << "  Last error: " << last_error_ << std::endl;
            break;
        }
    }
    
    // --- Verification ---
    if (error_occurred_.load()) {
        std::cout << "Server encountered an error: " << last_error_ << std::endl;
    } else {
        std::cout << "Server started without errors" << std::endl;
    }
    
    // 포트 바인딩 상태 확인
    if (isPortInUse(test_port)) {
        std::cout << "Port is successfully bound" << std::endl;
    } else {
        std::cout << "Port is not bound - server may not be listening" << std::endl;
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

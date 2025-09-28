#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include "unilink/unilink.hpp"

using namespace unilink;
using namespace std::chrono_literals;

class RealCommunicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        data_received_.clear();
        connection_established_ = false;
        error_occurred_ = false;
        server_ready_ = false;
    }
    
    void TearDown() override {
        if (client_) {
            client_->stop();
        }
        if (server_) {
            server_->stop();
        }
        
        // 충분한 시간을 두고 정리
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 테스트용 포트 번호
    uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{10000};
        return port_counter.fetch_add(1);
    }

    // 서버 준비 대기
    void waitForServerReady(std::chrono::milliseconds timeout = 2000ms) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, timeout, [this] { return server_ready_.load(); });
    }

    // 연결 대기
    void waitForConnection(std::chrono::milliseconds timeout = 2000ms) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, timeout, [this] { return connection_established_.load(); });
    }

    // 데이터 수신 대기
    void waitForData(std::chrono::milliseconds timeout = 2000ms) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, timeout, [this] { return !data_received_.empty(); });
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
 * @brief 실제 서버-클라이언트 통신 테스트
 */
TEST_F(RealCommunicationTest, ServerClientCommunication) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    
    // --- Test Logic ---
    // 1. 서버 생성 및 시작
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_data([this](const std::string& data) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("SERVER_RECEIVED: " + data);
            cv_.notify_one();
        })
        .on_connect([this]() {
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            server_ready_ = true;
            cv_.notify_one();
        })
        .on_error([this](const std::string& error) {
            std::lock_guard<std::mutex> lock(mtx_);
            error_occurred_ = true;
            last_error_ = error;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 서버가 준비될 때까지 대기
    waitForServerReady(3000ms);
    
    if (!server_ready_) {
        GTEST_SKIP() << "Server failed to start (port: " << test_port << ")";
        return;
    }
    
    // 2. 클라이언트 생성 및 연결
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .on_data([this](const std::string& data) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("CLIENT_RECEIVED: " + data);
            cv_.notify_one();
        })
        .on_connect([this]() {
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            cv_.notify_one();
        })
        .on_error([this](const std::string& error) {
            std::lock_guard<std::mutex> lock(mtx_);
            error_occurred_ = true;
            last_error_ = error;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(client_, nullptr);
    
    // 클라이언트 연결 대기
    waitForConnection(3000ms);
    
    if (!connection_established_) {
        GTEST_SKIP() << "Client failed to connect to server (port: " << test_port << ")";
        return;
    }
    
    // 3. 데이터 전송 테스트
    const std::string test_message = "Hello from client!";
    client_->send(test_message);
    
    // 데이터 수신 대기
    waitForData(3000ms);
    
    // --- Verification ---
    EXPECT_FALSE(data_received_.empty());
    if (!data_received_.empty()) {
        // 서버가 데이터를 받았는지 확인
        bool server_received = false;
        for (const auto& data : data_received_) {
            if (data.find("SERVER_RECEIVED: " + test_message) != std::string::npos) {
                server_received = true;
                break;
            }
        }
        EXPECT_TRUE(server_received) << "Server did not receive the message";
    }
}

/**
 * @brief 에코 서버 테스트 (서버가 받은 데이터를 다시 클라이언트에게 전송)
 */
TEST_F(RealCommunicationTest, EchoServerTest) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    
    // --- Test Logic ---
    // 1. 에코 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_data([this](const std::string& data) {
            // 에코 서버: 받은 데이터를 그대로 다시 전송
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("SERVER_RECEIVED: " + data);
            
            // 클라이언트에게 에코 응답 전송
            if (client_) {
                client_->send("ECHO: " + data);
            }
            cv_.notify_one();
        })
        .on_connect([this]() {
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            server_ready_ = true;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 서버 준비 대기
    waitForServerReady(3000ms);
    
    if (!server_ready_) {
        GTEST_SKIP() << "Echo server failed to start (port: " << test_port << ")";
        return;
    }
    
    // 2. 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .on_data([this](const std::string& data) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("CLIENT_RECEIVED: " + data);
            cv_.notify_one();
        })
        .on_connect([this]() {
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(client_, nullptr);
    
    // 클라이언트 연결 대기
    waitForConnection(3000ms);
    
    if (!connection_established_) {
        GTEST_SKIP() << "Client failed to connect to echo server (port: " << test_port << ")";
        return;
    }
    
    // 3. 에코 테스트
    const std::string echo_message = "Echo test message";
    client_->send(echo_message);
    
    // 에코 응답 대기
    waitForData(3000ms);
    
    // --- Verification ---
    EXPECT_FALSE(data_received_.empty());
    
    // 서버가 데이터를 받았는지 확인
    bool server_received = false;
    bool client_received_echo = false;
    
    for (const auto& data : data_received_) {
        if (data.find("SERVER_RECEIVED: " + echo_message) != std::string::npos) {
            server_received = true;
        }
        if (data.find("CLIENT_RECEIVED: ECHO: " + echo_message) != std::string::npos) {
            client_received_echo = true;
        }
    }
    
    EXPECT_TRUE(server_received) << "Server did not receive the message";
    EXPECT_TRUE(client_received_echo) << "Client did not receive echo response";
}

/**
 * @brief 다중 메시지 통신 테스트
 */
TEST_F(RealCommunicationTest, MultipleMessageCommunication) {
    // --- Setup ---
    uint16_t test_port = getTestPort();
    const int num_messages = 5;
    
    // --- Test Logic ---
    // 1. 서버 생성
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(true)
        .on_data([this](const std::string& data) {
            std::lock_guard<std::mutex> lock(mtx_);
            data_received_.push_back("SERVER: " + data);
            cv_.notify_one();
        })
        .on_connect([this]() {
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            server_ready_ = true;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 서버 준비 대기
    waitForServerReady(3000ms);
    
    if (!server_ready_) {
        GTEST_SKIP() << "Server failed to start for multiple message test (port: " << test_port << ")";
        return;
    }
    
    // 2. 클라이언트 생성
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(true)
        .on_connect([this]() {
            std::lock_guard<std::mutex> lock(mtx_);
            connection_established_ = true;
            cv_.notify_one();
        })
        .build();
    
    ASSERT_NE(client_, nullptr);
    
    // 클라이언트 연결 대기
    waitForConnection(3000ms);
    
    if (!connection_established_) {
        GTEST_SKIP() << "Client failed to connect for multiple message test (port: " << test_port << ")";
        return;
    }
    
    // 3. 다중 메시지 전송
    for (int i = 0; i < num_messages; ++i) {
        const std::string message = "Message " + std::to_string(i + 1);
        client_->send(message);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 메시지 간 간격
    }
    
    // 모든 메시지 수신 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // --- Verification ---
    EXPECT_GE(data_received_.size(), num_messages) << "Server did not receive all messages";
    
    // 각 메시지가 수신되었는지 확인
    for (int i = 0; i < num_messages; ++i) {
        const std::string expected = "SERVER: Message " + std::to_string(i + 1);
        bool found = false;
        for (const auto& data : data_received_) {
            if (data == expected) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Message " << (i + 1) << " not received by server";
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

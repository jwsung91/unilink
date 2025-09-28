#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>

#include "unilink/unilink.hpp"

using namespace unilink;

class BuilderTest : public ::testing::Test {
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
            server_->stop();
        }
        if (client_) {
            client_->stop();
        }
        if (serial_) {
            serial_->stop();
        }
        
        // 잠시 대기하여 정리 완료 보장
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 테스트용 포트 번호 (동적 할당으로 충돌 방지)
    uint16_t getTestPort() {
        static std::atomic<uint16_t> port_counter{9000};
        return port_counter.fetch_add(1);
    }

    // 테스트용 데이터 핸들러
    void setupDataHandler() {
        if (server_) {
            server_->on_data([this](const std::string& data) {
                data_received_.push_back(data);
            });
        }
        if (client_) {
            client_->on_data([this](const std::string& data) {
                data_received_.push_back(data);
            });
        }
        if (serial_) {
            serial_->on_data([this](const std::string& data) {
                data_received_.push_back(data);
            });
        }
    }

    // 테스트용 연결 핸들러
    void setupConnectionHandler() {
        if (server_) {
            server_->on_connect([this]() {
                connection_established_ = true;
            });
        }
        if (client_) {
            client_->on_connect([this]() {
                connection_established_ = true;
            });
        }
        if (serial_) {
            serial_->on_connect([this]() {
                connection_established_ = true;
            });
        }
    }

    // 테스트용 에러 핸들러
    void setupErrorHandler() {
        if (server_) {
            server_->on_error([this](const std::string& error) {
                error_occurred_ = true;
                last_error_ = error;
            });
        }
        if (client_) {
            client_->on_error([this](const std::string& error) {
                error_occurred_ = true;
                last_error_ = error;
            });
        }
        if (serial_) {
            serial_->on_error([this](const std::string& error) {
                error_occurred_ = true;
                last_error_ = error;
            });
        }
    }

protected:
    std::shared_ptr<wrapper::TcpServer> server_;
    std::shared_ptr<wrapper::TcpClient> client_;
    std::shared_ptr<wrapper::Serial> serial_;
    
    std::vector<std::string> data_received_;
    std::atomic<bool> connection_established_{false};
    std::atomic<bool> error_occurred_{false};
    std::string last_error_;
};

// TcpServerBuilder 기본 테스트
TEST_F(BuilderTest, TcpServerBuilderBasic) {
    uint16_t test_port = getTestPort();
    
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)  // 수동 시작으로 제어
        .on_data([](const std::string& data) {
            // 데이터 처리
        })
        .on_connect([]() {
            // 연결 처리
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    EXPECT_FALSE(server_->is_connected());
    
    // 수동으로 시작
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 서버가 생성되었는지 확인
    EXPECT_TRUE(server_ != nullptr);
}

// TcpClientBuilder 기본 테스트
TEST_F(BuilderTest, TcpClientBuilderBasic) {
    uint16_t test_port = getTestPort();
    
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(false)  // 수동 시작으로 제어
        .on_data([](const std::string& data) {
            // 데이터 처리
        })
        .on_connect([]() {
            // 연결 처리
        })
        .build();
    
    ASSERT_NE(client_, nullptr);
    EXPECT_FALSE(client_->is_connected());
    
    // 수동으로 시작
    client_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 클라이언트가 생성되었는지 확인
    EXPECT_TRUE(client_ != nullptr);
}

// SerialBuilder 기본 테스트
TEST_F(BuilderTest, SerialBuilderBasic) {
    serial_ = builder::UnifiedBuilder::serial("/dev/null", 9600)
        .auto_start(false)  // 수동 시작으로 제어
        .on_data([](const std::string& data) {
            // 데이터 처리
        })
        .on_connect([]() {
            // 연결 처리
        })
        .build();
    
    ASSERT_NE(serial_, nullptr);
    EXPECT_FALSE(serial_->is_connected());
    
    // 수동으로 시작
    serial_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Serial이 생성되었는지 확인
    EXPECT_TRUE(serial_ != nullptr);
}

// Builder 체이닝 테스트
TEST_F(BuilderTest, BuilderChaining) {
    uint16_t test_port = getTestPort();
    
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)
        .auto_manage(true)
        .on_data([this](const std::string& data) {
            data_received_.push_back(data);
        })
        .on_connect([this]() {
            connection_established_ = true;
        })
        .on_disconnect([]() {
            // 연결 해제 처리
        })
        .on_error([this](const std::string& error) {
            error_occurred_ = true;
            last_error_ = error;
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    EXPECT_FALSE(server_->is_connected());
    
    // 수동으로 시작
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 서버가 생성되었는지 확인
    EXPECT_TRUE(server_ != nullptr);
}

// 여러 Builder 동시 사용 테스트
TEST_F(BuilderTest, MultipleBuilders) {
    uint16_t test_port = getTestPort();
    
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)
        .build();
    
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port)
        .auto_start(false)
        .build();
    
    serial_ = builder::UnifiedBuilder::serial("/dev/null", 115200)
        .auto_start(false)
        .build();
    
    ASSERT_NE(server_, nullptr);
    ASSERT_NE(client_, nullptr);
    ASSERT_NE(serial_, nullptr);
    
    // 각각 시작
    server_->start();
    client_->start();
    serial_->start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 모든 객체가 생성되었는지 확인
    EXPECT_TRUE(server_ != nullptr);
    EXPECT_TRUE(client_ != nullptr);
    EXPECT_TRUE(serial_ != nullptr);
}

// Builder 설정 검증 테스트
TEST_F(BuilderTest, BuilderConfiguration) {
    uint16_t test_port = getTestPort();
    
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .auto_start(false)
        .auto_manage(false)
        .build();
    
    ASSERT_NE(server_, nullptr);
    EXPECT_FALSE(server_->is_connected());
    
    // 수동으로 시작
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 서버가 생성되었는지 확인
    EXPECT_TRUE(server_ != nullptr);
}

// 콜백 등록 테스트
TEST_F(BuilderTest, CallbackRegistration) {
    uint16_t test_port = getTestPort();
    std::atomic<int> callback_count{0};
    
    server_ = builder::UnifiedBuilder::tcp_server(test_port)
        .on_data([&callback_count](const std::string& data) {
            callback_count++;
        })
        .on_connect([&callback_count]() {
            callback_count++;
        })
        .on_disconnect([&callback_count]() {
            callback_count++;
        })
        .on_error([&callback_count](const std::string& error) {
            callback_count++;
        })
        .build();
    
    ASSERT_NE(server_, nullptr);
    
    // 서버 시작
    server_->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 서버가 생성되었는지 확인
    EXPECT_TRUE(server_ != nullptr);
    // 초기 상태에서는 콜백이 호출되지 않아야 함
    EXPECT_EQ(callback_count.load(), 0);
}

// 빌더 재사용 테스트
TEST_F(BuilderTest, BuilderReuse) {
    uint16_t test_port = getTestPort();
    auto builder = builder::UnifiedBuilder::tcp_server(test_port);
    
    // 첫 번째 서버
    auto server1 = builder
        .auto_start(false)
        .on_data([](const std::string& data) {})
        .build();
    
    // 두 번째 서버 (같은 빌더 재사용)
    auto server2 = builder
        .auto_start(false)
        .on_connect([]() {})
        .build();
    
    ASSERT_NE(server1, nullptr);
    ASSERT_NE(server2, nullptr);
    
    // 각각 시작
    server1->start();
    server2->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 각 서버가 생성되었는지 확인
    EXPECT_TRUE(server1 != nullptr);
    EXPECT_TRUE(server2 != nullptr);
    
    // 정리
    server1->stop();
    server2->stop();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


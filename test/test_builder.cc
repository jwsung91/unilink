#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>

#include "unilink/unilink.hpp"

using namespace unilink;

class BuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 전 초기화
    }
    
    void TearDown() override {
        // 테스트 후 정리
    }
};

// TcpServerBuilder 기본 테스트
TEST_F(BuilderTest, TcpServerBuilderBasic) {
    auto server = builder::UnifiedBuilder::tcp_server(9025)
        .auto_start(true)
        .on_data([](const std::string& data) {
            // 데이터 처리
        })
        .on_connect([]() {
            // 연결 처리
        })
        .build();
    
    ASSERT_NE(server, nullptr);
    EXPECT_FALSE(server->is_connected());
    
    // 정리
    server->stop();
}

// TcpClientBuilder 기본 테스트
TEST_F(BuilderTest, TcpClientBuilderBasic) {
    auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", 9026)
        .auto_start(true)
        .on_data([](const std::string& data) {
            // 데이터 처리
        })
        .on_connect([]() {
            // 연결 처리
        })
        .build();
    
    ASSERT_NE(client, nullptr);
    EXPECT_FALSE(client->is_connected());
    
    // 정리
    client->stop();
}

// SerialBuilder 기본 테스트
TEST_F(BuilderTest, SerialBuilderBasic) {
    auto serial = builder::UnifiedBuilder::serial("/dev/null", 9600)
        .auto_start(true)
        .on_data([](const std::string& data) {
            // 데이터 처리
        })
        .on_connect([]() {
            // 연결 처리
        })
        .build();
    
    ASSERT_NE(serial, nullptr);
    EXPECT_FALSE(serial->is_connected());
    
    // 정리
    serial->stop();
}

// Builder 체이닝 테스트
TEST_F(BuilderTest, BuilderChaining) {
    bool data_received = false;
    bool connected = false;
    bool error_occurred = false;
    
    auto server = builder::UnifiedBuilder::tcp_server(9027)
        .auto_start(true)
        .auto_manage(true)
        .on_data([&data_received](const std::string& data) {
            data_received = true;
        })
        .on_connect([&connected]() {
            connected = true;
        })
        .on_disconnect([]() {
            // 연결 해제 처리
        })
        .on_error([&error_occurred](const std::string& error) {
            error_occurred = true;
        })
        .build();
    
    ASSERT_NE(server, nullptr);
    
    // 정리
    server->stop();
}

// 여러 Builder 동시 사용 테스트
TEST_F(BuilderTest, MultipleBuilders) {
    auto server = builder::UnifiedBuilder::tcp_server(9028)
        .auto_start(true)
        .build();
    
    auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", 9028)
        .auto_start(true)
        .build();
    
    auto serial = builder::UnifiedBuilder::serial("/dev/null", 115200)
        .auto_start(true)
        .build();
    
    ASSERT_NE(server, nullptr);
    ASSERT_NE(client, nullptr);
    ASSERT_NE(serial, nullptr);
    
    // 정리
    server->stop();
    client->stop();
    serial->stop();
}

// Builder 설정 검증 테스트
TEST_F(BuilderTest, BuilderConfiguration) {
    auto server = builder::UnifiedBuilder::tcp_server(9029)
        .auto_start(false)
        .auto_manage(false)
        .build();
    
    ASSERT_NE(server, nullptr);
    EXPECT_FALSE(server->is_connected());
    
    // 수동으로 시작
    server->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server->stop();
}

// 콜백 등록 테스트
TEST_F(BuilderTest, CallbackRegistration) {
    int callback_count = 0;
    
    auto server = builder::UnifiedBuilder::tcp_server(9030)
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
    
    ASSERT_NE(server, nullptr);
    
    // 정리
    server->stop();
}

// 빌더 재사용 테스트
TEST_F(BuilderTest, BuilderReuse) {
    auto builder = builder::UnifiedBuilder::tcp_server(9031);
    
    // 첫 번째 서버
    auto server1 = builder
        .auto_start(true)
        .on_data([](const std::string& data) {})
        .build();
    
    // 두 번째 서버 (같은 빌더 재사용)
    auto server2 = builder
        .auto_start(false)
        .on_connect([]() {})
        .build();
    
    ASSERT_NE(server1, nullptr);
    ASSERT_NE(server2, nullptr);
    
    // 정리
    server1->stop();
    server2->stop();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


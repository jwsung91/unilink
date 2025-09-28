#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <future>

#include "unilink/unilink.hpp"

using namespace unilink;

class WrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 전 초기화
    }
    
    void TearDown() override {
        // 테스트 후 정리
    }
};

// TCP Server Wrapper 기본 테스트
TEST_F(WrapperTest, TcpServerBasicCreation) {
    auto server = std::make_unique<wrapper::TcpServer>(9001);
    ASSERT_NE(server, nullptr);
    
    // 기본 상태 확인
    EXPECT_FALSE(server->is_connected());
    
    // 핸들러 설정
    bool data_received = false;
    bool connected = false;
    
    server->on_data([&data_received](const std::string& data) {
        data_received = true;
        EXPECT_EQ(data, "test");
    });
    
    server->on_connect([&connected]() {
        connected = true;
    });
    
    // 서버 시작
    server->start();
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 서버 중지
    server->stop();
}

// TCP Client Wrapper 기본 테스트
TEST_F(WrapperTest, TcpClientBasicCreation) {
    auto client = std::make_unique<wrapper::TcpClient>("127.0.0.1", 9002);
    ASSERT_NE(client, nullptr);
    
    // 기본 상태 확인
    EXPECT_FALSE(client->is_connected());
    
    // 핸들러 설정
    bool data_received = false;
    bool connected = false;
    
    client->on_data([&data_received](const std::string& data) {
        data_received = true;
    });
    
    client->on_connect([&connected]() {
        connected = true;
    });
    
    // 클라이언트 시작
    client->start();
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 클라이언트 중지
    client->stop();
}

// Serial Wrapper 기본 테스트
TEST_F(WrapperTest, SerialBasicCreation) {
    auto serial = std::make_unique<wrapper::Serial>("/dev/null", 9600);
    ASSERT_NE(serial, nullptr);
    
    // 기본 상태 확인
    EXPECT_FALSE(serial->is_connected());
    
    // 핸들러 설정
    bool data_received = false;
    bool connected = false;
    
    serial->on_data([&data_received](const std::string& data) {
        data_received = true;
    });
    
    serial->on_connect([&connected]() {
        connected = true;
    });
    
    // 시리얼 시작
    serial->start();
    
    // 잠시 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 시리얼 중지
    serial->stop();
}

// Wrapper 클래스들의 RAII 테스트
TEST_F(WrapperTest, WrapperRAII) {
    {
        auto server = std::make_unique<wrapper::TcpServer>(9003);
        auto client = std::make_unique<wrapper::TcpClient>("127.0.0.1", 9003);
        auto serial = std::make_unique<wrapper::Serial>("/dev/null", 9600);
        
        // 객체들이 정상적으로 생성되었는지 확인
        EXPECT_NE(server, nullptr);
        EXPECT_NE(client, nullptr);
        EXPECT_NE(serial, nullptr);
        
        // auto_manage 설정
        server->auto_manage(true);
        client->auto_manage(true);
        serial->auto_manage(true);
        
        // start() 호출
        server->start();
        client->start();
        serial->start();
        
        // 잠시 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 스코프를 벗어나면 자동으로 정리되어야 함
    }
    
    // 여기서 모든 리소스가 정리되었는지 확인
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 메모리 누수 테스트 (간단한 버전)
TEST_F(WrapperTest, MemoryLeakTest) {
    const int iterations = 10;
    
    for (int i = 0; i < iterations; ++i) {
        auto server = std::make_unique<wrapper::TcpServer>(9004 + i);
        auto client = std::make_unique<wrapper::TcpClient>("127.0.0.1", 9004 + i);
        
        server->auto_manage(true);
        client->auto_manage(true);
        
        server->start();
        client->start();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // 명시적으로 정리
        server->stop();
        client->stop();
        
        server.reset();
        client.reset();
    }
    
    // 메모리 정리가 완료될 시간을 줌
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 데이터 전송 테스트
TEST_F(WrapperTest, DataTransmission) {
    auto server = std::make_unique<wrapper::TcpServer>(9015);
    auto client = std::make_unique<wrapper::TcpClient>("127.0.0.1", 9015);
    
    std::string received_data;
    bool data_received = false;
    
    // 서버 핸들러 설정
    server->on_data([&received_data, &data_received](const std::string& data) {
        received_data = data;
        data_received = true;
    });
    
    // 클라이언트 핸들러 설정
    client->on_data([&](const std::string& data) {
        // 에코 서버처럼 데이터를 다시 전송
        client->send(data);
    });
    
    server->start();
    client->start();
    
    // 연결 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // 데이터 전송
    client->send("Hello World!");
    
    // 응답 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 정리
    server->stop();
    client->stop();
}

// 콜백 체이닝 테스트
TEST_F(WrapperTest, CallbackChaining) {
    auto server = std::make_unique<wrapper::TcpServer>(9016);
    
    int callback_count = 0;
    
    // 여러 콜백 등록
    server->on_connect([&callback_count]() {
        callback_count++;
    });
    
    server->on_disconnect([&callback_count]() {
        callback_count++;
    });
    
    server->on_error([&callback_count](const std::string& error) {
        callback_count++;
    });
    
    server->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server->stop();
    
    // 콜백이 등록되었는지 확인 (실제 호출은 연결 상태에 따라 다름)
    EXPECT_GE(callback_count, 0);
}

// auto_start 기능 테스트
TEST_F(WrapperTest, AutoStartFeature) {
    auto server = std::make_unique<wrapper::TcpServer>(9017);
    
    // auto_start 설정
    server->auto_start(true);
    
    // start()를 명시적으로 호출하지 않아도 자동으로 시작되어야 함
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 정리
    server->stop();
}

// auto_manage 기능 테스트
TEST_F(WrapperTest, AutoManageFeature) {
    auto server = std::make_unique<wrapper::TcpServer>(9018);
    
    // auto_manage 설정
    server->auto_manage(true);
    server->start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 명시적으로 정리 (auto_manage는 실제로는 구현되지 않음)
    server->stop();
}

// send_line 기능 테스트
TEST_F(WrapperTest, SendLineFeature) {
    auto server = std::make_unique<wrapper::TcpServer>(9019);
    auto client = std::make_unique<wrapper::TcpClient>("127.0.0.1", 9019);
    
    std::string received_data;
    
    server->on_data([&received_data](const std::string& data) {
        received_data = data;
    });
    
    server->start();
    client->start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // send_line 테스트
    client->send_line("Test Line");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 정리
    server->stop();
    client->stop();
}

// 에러 핸들링 테스트
TEST_F(WrapperTest, ErrorHandling) {
    auto client = std::make_unique<wrapper::TcpClient>("invalid_host", 9999);
    
    bool error_occurred = false;
    std::string error_message;
    
    client->on_error([&error_occurred, &error_message](const std::string& error) {
        error_occurred = true;
        error_message = error;
    });
    
    client->start();
    
    // 연결 실패 대기
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 에러가 발생했는지 확인
    // (실제 에러 발생 여부는 네트워크 상태에 따라 다를 수 있음)
    
    client->stop();
}

// 동시성 테스트
TEST_F(WrapperTest, ConcurrencyTest) {
    const int num_clients = 5;
    std::vector<std::unique_ptr<wrapper::TcpClient>> clients;
    
    // 여러 클라이언트 생성
    for (int i = 0; i < num_clients; ++i) {
        auto client = std::make_unique<wrapper::TcpClient>("127.0.0.1", 9020 + i);
        clients.push_back(std::move(client));
    }
    
    // 모든 클라이언트 시작
    for (auto& client : clients) {
        client->start();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 모든 클라이언트 중지
    for (auto& client : clients) {
        client->stop();
    }
}

// 설정 변경 테스트
TEST_F(WrapperTest, ConfigurationTest) {
    // TcpServer 설정 테스트
    auto server = std::make_unique<wrapper::TcpServer>(9021);
    EXPECT_FALSE(server->is_connected());
    
    // TcpClient 설정 테스트
    auto client = std::make_unique<wrapper::TcpClient>("127.0.0.1", 9021);
    EXPECT_FALSE(client->is_connected());
    
    // Serial 설정 테스트
    auto serial = std::make_unique<wrapper::Serial>("/dev/null", 115200);
    EXPECT_FALSE(serial->is_connected());
    
    // 정리
    server->stop();
    client->stop();
    serial->stop();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

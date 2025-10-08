#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

#include "unilink/unilink.hpp"

std::atomic<bool> running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[client] Received shutdown signal" << std::endl;
        running = false;
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "사용법: " << argv[0] << " <server_ip> <port>" << std::endl;
        std::cout << "예시: " << argv[0] << " 127.0.0.1 8080" << std::endl;
        return 1;
    }
    
    std::string server_ip = argv[1];
    unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));
    
    // 시그널 핸들러 설정
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "=== 멀티 클라이언트 TCP 채팅 클라이언트 ===" << std::endl;
    std::cout << "서버: " << server_ip << ":" << port << std::endl;
    std::cout << "종료: Ctrl+C 또는 /quit 입력" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // TCP 클라이언트 생성
    auto client = unilink::tcp_client(server_ip, port)
        .on_connect([]() {
            std::cout << "[client] 서버에 연결됨" << std::endl;
        })
        .on_disconnect([]() {
            std::cout << "[client] 서버와 연결 해제됨" << std::endl;
        })
        .on_data([](const std::string& data) {
            std::cout << "[RX] " << data << std::endl;
        })
        .on_error([](const std::string& error) {
            std::cout << "[client] 오류: " << error << std::endl;
        })
        .auto_start(true)
        .build();
    
    if (!client) {
        std::cerr << "[client] 클라이언트 생성 실패" << std::endl;
        return 1;
    }
    
    // 연결 대기
    std::cout << "[client] 서버 연결 시도 중..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (!client->is_connected()) {
        std::cerr << "[client] 서버 연결 실패" << std::endl;
        return 1;
    }
    
    std::cout << "[client] 연결 성공! 메시지를 입력하세요." << std::endl;
    
    // 사용자 입력 처리
    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        if (line == "/quit" || line == "/exit") {
            std::cout << "[client] 연결 종료 중..." << std::endl;
            break;
        } else if (line == "/status") {
            std::cout << "[client] 연결 상태: " << (client->is_connected() ? "연결됨" : "연결 해제됨") << std::endl;
        } else {
            // 서버에게 메시지 전송
            client->send_line(line);
            std::cout << "[TX] " << line << std::endl;
        }
    }
    
    // 정리
    std::cout << "[client] 클라이언트 종료 중..." << std::endl;
    
    if (client) {
        client->stop();
    }
    
    std::cout << "[client] 클라이언트 종료 완료" << std::endl;
    return 0;
}

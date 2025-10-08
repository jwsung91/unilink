#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

#include "unilink/unilink.hpp"
#include "unilink/common/logger.hpp"

// 전역 변수로 running 선언 (스레드 안전)
std::atomic<bool> running{true};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        if (running.load()) {
            auto& logger = unilink::common::Logger::instance();
            logger.info("server", "signal", "Received shutdown signal");
            running.store(false);
        } else {
            // 이미 종료 중이면 강제 종료
            auto& logger = unilink::common::Logger::instance();
            logger.warning("server", "signal", "Force exit...");
            std::_Exit(1);
        }
    }
}

int main(int argc, char** argv) {
    unsigned short port = (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 8080;
    
    // 시그널 핸들러 설정
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Logger 초기화
    auto& logger = unilink::common::Logger::instance();
    logger.set_level(unilink::common::LogLevel::INFO);
    logger.set_console_output(true);
    
    std::cout << "=== Multi-Client TCP Chat Server ===" << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Exit: Ctrl+C or /quit" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  /clients - Show connected clients" << std::endl;
    std::cout << "  /send <id> <message> - Send to specific client" << std::endl;
    std::cout << "  <message> - Broadcast to all clients" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // TCP 서버 생성
    auto server = unilink::tcp_server(port)
        .on_multi_connect([&logger](int client_id, const std::string& client_ip) {
            logger.info("server", "connect", "Client " + std::to_string(client_id) + " connected: " + client_ip);
        })
        .on_multi_data([&logger](int client_id, const std::string& data) {
            logger.info("server", "data", "Client " + std::to_string(client_id) + " message: " + data);
        })
        .on_multi_disconnect([&logger](int client_id) {
            logger.info("server", "disconnect", "Client " + std::to_string(client_id) + " disconnected");
        })
        .auto_start(true)
        .build();
    
    if (!server) {
        logger.error("server", "startup", "Failed to create server");
        return 1;
    }
    
    // 서버 시작 확인
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // 서버 시작 확인을 제거하고 바로 진행
    
    logger.info("server", "startup", "Server started. Waiting for client connections...");
    
    // 입력 스레드
    std::thread input_thread([&server, &logger]() {
        std::string line;
        while (running.load()) {
            if (std::getline(std::cin, line)) {
                if (line.empty()) continue;
                
                if (line == "/quit" || line == "/exit") {
                    logger.info("server", "shutdown", "Shutting down server...");
                    running.store(false);
                    break;
                } else if (line == "/clients") {
                    int count = server->get_client_count();
                    logger.info("server", "status", std::to_string(count) + " clients connected");
                } else if (line.substr(0, 5) == "/send") {
                    // /send <id> <message> 형식 처리
                    size_t first_space = line.find(' ', 6);
                    if (first_space != std::string::npos) {
                        try {
                            int client_id = std::stoi(line.substr(6, first_space - 6));
                            std::string message = line.substr(first_space + 1);
                            server->send_to_client(client_id, message);
                            logger.info("server", "send", "Sent to client " + std::to_string(client_id) + ": " + message);
                        } catch (const std::exception& e) {
                            logger.error("server", "send", "Invalid send command format");
                        }
                    } else {
                        logger.error("server", "send", "Invalid send command format");
                    }
                } else {
                    // 브로드캐스트
                    server->broadcast(line);
                    logger.info("server", "broadcast", "Broadcast to all clients: " + line);
                }
            } else {
                // EOF or error on stdin, break the loop
                break;
            }
        }
    });
    
    // 메인 루프
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    logger.info("server", "shutdown", "Shutting down server...");
    
    if (server) {
        // 클라이언트에게 종료 알림 전송
        server->broadcast("[Server] Server is shutting down. Please disconnect.");
        logger.info("server", "shutdown", "Notified all clients about shutdown");
        
        // 클라이언트가 메시지를 받을 시간 제공
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 서버 중지
        server->stop();
        logger.info("server", "shutdown", "Server stopped");
        
        // 서버 정리 대기
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Wait for threads to finish with timeout
    if (input_thread.joinable()) {
        input_thread.join();
    }
    
    logger.info("server", "shutdown", "Server shutdown complete");
    
    // 확실한 종료를 위해 _Exit 호출
    std::_Exit(0);
}
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
            std::exit(1);
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
            logger.info("server", "debug", "Multi-connect callback triggered for client " + std::to_string(client_id));
        })
        .on_multi_data([&logger](int client_id, const std::string& data) {
            logger.info("server", "data", "Client " + std::to_string(client_id) + " message: " + data);
            logger.info("server", "debug", "Data length: " + std::to_string(data.length()));
            logger.info("server", "debug", "Data bytes: " + std::to_string(data.size()));
        })
        .on_multi_disconnect([&logger](int client_id) {
            logger.info("server", "disconnect", "Client " + std::to_string(client_id) + " disconnected");
        })
        .enable_port_retry(true, 3, 1000)  // 3회 재시도, 1초 간격
        .auto_start(false)
        .build();
    
    if (!server) {
        logger.error("server", "startup", "Failed to create server");
        return 1;
    }
    
    // 브로드캐스트 기능을 위한 콜백 재설정
    server->on_multi_connect([&logger, &server](int client_id, const std::string& client_ip) {
        logger.info("server", "connect", "Client " + std::to_string(client_id) + " connected: " + client_ip);
        logger.info("server", "debug", "Multi-connect callback triggered for client " + std::to_string(client_id));
        
        // 클라이언트 연결 시 브로드캐스팅
        server->broadcast("[Server] Client " + std::to_string(client_id) + " (" + client_ip + ") joined the chat!");
        logger.info("server", "broadcast", "Broadcasted client join message for client " + std::to_string(client_id));
    });
    
    server->on_multi_data([&logger, &server](int client_id, const std::string& data) {
        logger.info("server", "data", "Client " + std::to_string(client_id) + " message: " + data);
        logger.info("server", "debug", "Data length: " + std::to_string(data.length()));
        logger.info("server", "debug", "Data bytes: " + std::to_string(data.size()));
        
        // 클라이언트 메시지를 모든 클라이언트에게 브로드캐스팅
        server->broadcast("[Client " + std::to_string(client_id) + "] " + data);
        logger.info("server", "broadcast", "Broadcasted message from client " + std::to_string(client_id) + " to all clients");
    });
    
    server->on_multi_disconnect([&logger, &server](int client_id) {
        logger.info("server", "disconnect", "Client " + std::to_string(client_id) + " disconnected");
        
        // 클라이언트 연결 해제 시 브로드캐스팅
        server->broadcast("[Server] Client " + std::to_string(client_id) + " left the chat!");
        logger.info("server", "broadcast", "Broadcasted client leave message for client " + std::to_string(client_id));
    });
    
    // 서버 시작
    server->start();
    
    // 서버 시작 확인 (재시도 시간 고려: 3회 재시도 * 1초 + 0.5초 버퍼)
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + (3 * 1000)));
    
    // Check if server started successfully
    if (!server->is_listening()) {
        logger.error("server", "startup", "Failed to start server - port may be in use");
        return 1;
    }
    
    logger.info("server", "startup", "Server started. Waiting for client connections...");
    
    // 브로드캐스트 테스트를 위한 타이머 설정
    std::thread broadcast_timer([&server, &logger]() {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 5초 후 브로드캐스트
        logger.info("server", "debug", "Broadcast timer triggered");
        if (server) {
            int client_count = server->get_client_count();
            logger.info("server", "debug", "Client count: " + std::to_string(client_count));
            if (client_count > 0) {
                server->broadcast("[Server] Welcome to the chat server!");
                logger.info("server", "broadcast", "Sent welcome message to all clients");
                
                // 서버 메시지 브로드캐스트 테스트
                std::this_thread::sleep_for(std::chrono::seconds(3));
                server->broadcast("[Server] This is a test message from server!");
                logger.info("server", "broadcast", "Sent test message to all clients");
            } else {
                logger.info("server", "debug", "No clients connected, skipping broadcast");
            }
        } else {
            logger.info("server", "debug", "Server is null, skipping broadcast");
        }
    });
    broadcast_timer.detach();
    
    // 메인 루프 (입력 처리 포함)
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 비블로킹 입력 처리 - 더 강력한 방법
        if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() != EOF) {
            std::string line;
            if (std::getline(std::cin, line)) {
                logger.info("server", "debug", "Received input: '" + line + "'");
                if (!line.empty()) {
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
                                
                                // 개별 클라이언트에게 전송 후 브로드캐스트
                                server->send_to_client(client_id, message);
                                server->broadcast("[Server] -> Client " + std::to_string(client_id) + ": " + message);
                                logger.info("server", "send", "Sent to client " + std::to_string(client_id) + ": " + message);
                                logger.info("server", "broadcast", "Broadcasted server message to all clients");
                            } catch (const std::exception& e) {
                                logger.error("server", "send", "Invalid send command format");
                            }
                        } else {
                            logger.error("server", "send", "Invalid send command format");
                        }
                    } else {
                        // 서버 메시지 브로드캐스트
                        logger.info("server", "debug", "Broadcasting server message: " + line);
                        server->broadcast("[Server] " + line);
                        logger.info("server", "broadcast", "Broadcast server message to all clients: " + line);
                    }
                }
            } else {
                // EOF or error on stdin, break the loop
                break;
            }
        }
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
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    // No input thread to wait for
    
    logger.info("server", "shutdown", "Server shutdown complete");
    
    // 정상 종료
    return 0;
}
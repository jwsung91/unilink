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
        if (running.load()) {
            std::cout << "\n[client] Received shutdown signal" << std::endl;
            running.store(false);
        } else {
            // 이미 종료 중이면 강제 종료
            std::cout << "\n[client] Force exit..." << std::endl;
            std::_Exit(1);
        }
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
    
    std::cout << "=== Multi-Client TCP Chat Client ===" << std::endl;
    std::cout << "Server: " << server_ip << ":" << port << std::endl;
    std::cout << "Exit: Ctrl+C or type /quit" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // 재연결 로직
    while (running.load()) {
        std::cout << "[client] Attempting to connect to server..." << std::endl;
        
        // TCP 클라이언트 생성 (3초 재연결 간격 설정)
        auto client = unilink::tcp_client(server_ip, port)
            .on_connect([]() {
                std::cout << "[client] Connected to server" << std::endl;
            })
            .on_disconnect([]() {
                std::cout << "[client] Disconnected from server" << std::endl;
            })
            .on_data([](const std::string& data) {
                std::cout << "[RX] " << data << std::endl;
                std::cout << "[DEBUG] Received data length: " << data.length() << std::endl;
                std::cout << "[DEBUG] Received data size: " << data.size() << std::endl;
                
                // 서버 종료 알림을 받으면 클라이언트도 종료
                if (data.find("[Server] Server is shutting down") != std::string::npos) {
                    std::cout << "[client] Server is shutting down. Disconnecting..." << std::endl;
                    running.store(false);
                }
            })
            .on_error([](const std::string& error) {
                std::cout << "[client] Error: " << error << std::endl;
            })
            .auto_start(true)
            .retry_interval(3000)  // 3초 재연결 간격 설정
            .build();
        
        if (!client) {
            std::cerr << "[client] Failed to create client" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }
        
        // 연결 대기 (최대 5초)
        bool connected = false;
        for (int i = 0; i < 50; ++i) {
            if (running.load() == false) {
                std::cout << "[client] Connection interrupted" << std::endl;
                return 0;
            }
            if (client->is_connected()) {
                connected = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (!connected) {
            std::cerr << "[client] Failed to connect to server. Retrying in 3 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }
        
        std::cout << "[client] Connection successful! Enter messages:" << std::endl;
        
        // 사용자 입력 처리 (비블로킹)
        std::string line;
        while (running.load() && client->is_connected()) {
            // 비블로킹 입력 처리 - 더 강력한 방법
            if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() != EOF) {
                if (std::getline(std::cin, line)) {
                    std::cout << "[DEBUG] Client received input: '" << line << "'" << std::endl;
                    if (line.empty()) continue;
                    
                    if (line == "/quit" || line == "/exit") {
                        std::cout << "[client] Disconnecting..." << std::endl;
                        if (client) {
                            client->stop();
                        }
                        running.store(false);
                        break;
                    } else if (line == "/status") {
                        std::cout << "[client] Connection status: " << (client->is_connected() ? "Connected" : "Disconnected") << std::endl;
                    } else {
                        // Send message to server
                        std::cout << "[DEBUG] Sending message to server: '" << line << "'" << std::endl;
                        client->send(line);
                        std::cout << "[TX] " << line << std::endl;
                    }
                } else {
                    // EOF or error on stdin, break the loop
                    break;
                }
            } else {
                // 짧은 대기 후 다시 체크
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        // 연결이 끊어졌을 때 재연결 시도
        if (running.load() && !client->is_connected()) {
            std::cout << "[client] Connection lost. Attempting to reconnect..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }
    
    // Cleanup
    std::cout << "[client] Shutting down client..." << std::endl;
    
    std::cout << "[client] Client shutdown complete" << std::endl;
    return 0;
}
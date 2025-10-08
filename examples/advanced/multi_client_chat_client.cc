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
    
    std::cout << "=== Multi-Client TCP Chat Client ===" << std::endl;
    std::cout << "Server: " << server_ip << ":" << port << std::endl;
    std::cout << "Exit: Ctrl+C or type /quit" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // TCP 클라이언트 생성
    auto client = unilink::tcp_client(server_ip, port)
        .on_connect([]() {
            std::cout << "[client] Connected to server" << std::endl;
        })
        .on_disconnect([]() {
            std::cout << "[client] Disconnected from server" << std::endl;
        })
        .on_data([](const std::string& data) {
            std::cout << "[RX] " << data << std::endl;
        })
        .on_error([](const std::string& error) {
            std::cout << "[client] Error: " << error << std::endl;
        })
        .auto_start(true)
        .build();
    
    if (!client) {
        std::cerr << "[client] Failed to create client" << std::endl;
        return 1;
    }
    
    // Wait for connection
    std::cout << "[client] Attempting to connect to server..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (!client->is_connected()) {
        std::cerr << "[client] Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "[client] Connection successful! Enter messages:" << std::endl;
    
    // 사용자 입력 처리
    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        if (line == "/quit" || line == "/exit") {
            std::cout << "[client] Disconnecting..." << std::endl;
            break;
        } else if (line == "/status") {
            std::cout << "[client] Connection status: " << (client->is_connected() ? "Connected" : "Disconnected") << std::endl;
        } else {
            // Send message to server
            client->send_line(line);
            std::cout << "[TX] " << line << std::endl;
        }
    }
    
    // Cleanup
    std::cout << "[client] Shutting down client..." << std::endl;
    
    if (client) {
        client->stop();
    }
    
    std::cout << "[client] Client shutdown complete" << std::endl;
    return 0;
}

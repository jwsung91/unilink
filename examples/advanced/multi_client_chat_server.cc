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
        std::cout << "\n[server] Received shutdown signal" << std::endl;
        running = false;
    }
}

int main(int argc, char** argv) {
    unsigned short port = (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 8080;
    
    // 시그널 핸들러 설정
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "=== Multi-Client TCP Chat Server ===" << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Exit: Ctrl+C" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // 멀티 클라이언트 TCP 서버 생성
    auto server = unilink::tcp_server(port)
        .on_multi_connect([](size_t client_id, const std::string& client_info) {
            std::cout << "[server] Client " << client_id << " connected: " << client_info << std::endl;
        })
        .on_multi_data([](size_t client_id, const std::string& data) {
            std::cout << "[server] Client " << client_id << " message: " << data << std::endl;
        })
        .on_multi_disconnect([](size_t client_id) {
            std::cout << "[server] Client " << client_id << " disconnected" << std::endl;
        })
        .auto_start(true)
        .build();
    
    if (!server) {
        std::cerr << "[server] Failed to create server" << std::endl;
        return 1;
    }
    
    std::cout << "[server] Server started. Waiting for client connections..." << std::endl;
    
    // 서버 상태 모니터링 스레드
    std::thread monitor_thread([&server, &running]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (server) {
                size_t client_count = server->get_client_count();
                auto connected_clients = server->get_connected_clients();
                
                std::cout << "[server] Status: " << client_count << " clients connected";
                if (!connected_clients.empty()) {
                    std::cout << " (ID: ";
                    for (size_t i = 0; i < connected_clients.size(); ++i) {
                        std::cout << connected_clients[i];
                        if (i < connected_clients.size() - 1) std::cout << ", ";
                    }
                    std::cout << ")";
                }
                std::cout << std::endl;
            }
        }
    });
    
    // 사용자 입력 처리 스레드
    std::thread input_thread([&server, &running]() {
        std::string line;
        while (running && std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            if (line == "/quit" || line == "/exit") {
                std::cout << "[server] Shutting down server..." << std::endl;
                running = false;
                break;
            } else if (line == "/clients") {
                if (server) {
                    size_t client_count = server->get_client_count();
                    auto connected_clients = server->get_connected_clients();
                    std::cout << "[server] Connected clients: " << client_count << std::endl;
                    for (auto client_id : connected_clients) {
                        std::cout << "  - Client " << client_id << std::endl;
                    }
                }
            } else if (line.substr(0, 6) == "/send ") {
                // Send message to specific client: /send <client_id> <message>
                size_t space_pos = line.find(' ', 6);
                if (space_pos != std::string::npos) {
                    try {
                        size_t client_id = std::stoul(line.substr(6, space_pos - 6));
                        std::string message = line.substr(space_pos + 1);
                        
                        if (server) {
                            server->send_to_client(client_id, "[Server] " + message);
                            std::cout << "[server] Sent to client " << client_id << ": " << message << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cout << "[server] Invalid command format: " << e.what() << std::endl;
                    }
                } else {
                    std::cout << "[server] Usage: /send <client_id> <message>" << std::endl;
                }
            } else {
                // Broadcast to all clients
                if (server) {
                    server->broadcast("[Server] " + line);
                    std::cout << "[server] Broadcast to all clients: " << line << std::endl;
                }
            }
        }
    });
    
    // 메인 루프
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    std::cout << "[server] Shutting down server..." << std::endl;
    
    if (server) {
        server->broadcast("[Server] Server is shutting down.");
        server->stop();
    }
    
    // Wait for threads to finish
    if (input_thread.joinable()) {
        input_thread.join();
    }
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
    
    std::cout << "[server] Server shutdown complete" << std::endl;
    return 0;
}

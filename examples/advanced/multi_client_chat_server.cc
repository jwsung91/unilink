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
    
    std::cout << "=== 멀티 클라이언트 TCP 채팅 서버 ===" << std::endl;
    std::cout << "포트: " << port << std::endl;
    std::cout << "종료: Ctrl+C" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // 멀티 클라이언트 TCP 서버 생성
    auto server = unilink::tcp_server(port)
        .on_multi_connect([](size_t client_id, const std::string& client_info) {
            std::cout << "[server] 클라이언트 " << client_id << " 연결됨: " << client_info << std::endl;
        })
        .on_multi_data([](size_t client_id, const std::string& data) {
            std::cout << "[server] 클라이언트 " << client_id << " 메시지: " << data << std::endl;
        })
        .on_multi_disconnect([](size_t client_id) {
            std::cout << "[server] 클라이언트 " << client_id << " 연결 해제됨" << std::endl;
        })
        .auto_start(true)
        .build();
    
    if (!server) {
        std::cerr << "[server] 서버 생성 실패" << std::endl;
        return 1;
    }
    
    std::cout << "[server] 서버 시작됨. 클라이언트 연결 대기 중..." << std::endl;
    
    // 서버 상태 모니터링 스레드
    std::thread monitor_thread([&server, &running]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            if (server) {
                size_t client_count = server->get_client_count();
                auto connected_clients = server->get_connected_clients();
                
                std::cout << "[server] 상태: " << client_count << "개 클라이언트 연결됨";
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
                std::cout << "[server] 서버 종료 중..." << std::endl;
                running = false;
                break;
            } else if (line == "/clients") {
                if (server) {
                    size_t client_count = server->get_client_count();
                    auto connected_clients = server->get_connected_clients();
                    std::cout << "[server] 연결된 클라이언트: " << client_count << "개" << std::endl;
                    for (auto client_id : connected_clients) {
                        std::cout << "  - 클라이언트 " << client_id << std::endl;
                    }
                }
            } else if (line.substr(0, 6) == "/send ") {
                // 특정 클라이언트에게 메시지 전송: /send <client_id> <message>
                size_t space_pos = line.find(' ', 6);
                if (space_pos != std::string::npos) {
                    try {
                        size_t client_id = std::stoul(line.substr(6, space_pos - 6));
                        std::string message = line.substr(space_pos + 1);
                        
                        if (server) {
                            server->send_to_client(client_id, "[서버] " + message);
                            std::cout << "[server] 클라이언트 " << client_id << "에게 전송: " << message << std::endl;
                        }
                    } catch (const std::exception& e) {
                        std::cout << "[server] 잘못된 명령어 형식: " << e.what() << std::endl;
                    }
                } else {
                    std::cout << "[server] 사용법: /send <client_id> <message>" << std::endl;
                }
            } else {
                // 모든 클라이언트에게 브로드캐스트
                if (server) {
                    server->broadcast("[서버] " + line);
                    std::cout << "[server] 모든 클라이언트에게 브로드캐스트: " << line << std::endl;
                }
            }
        }
    });
    
    // 메인 루프
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 정리
    std::cout << "[server] 서버 종료 중..." << std::endl;
    
    if (server) {
        server->broadcast("[서버] 서버가 종료됩니다.");
        server->stop();
    }
    
    // 스레드 종료 대기
    if (input_thread.joinable()) {
        input_thread.join();
    }
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
    
    std::cout << "[server] 서버 종료 완료" << std::endl;
    return 0;
}

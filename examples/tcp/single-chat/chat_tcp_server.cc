#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "unilink/unilink.hpp"

/**
 * @brief Chat application class for TCP server
 *
 * This class demonstrates member function binding for TCP server chat functionality.
 */
class TcpServerChatApp {
 public:
  TcpServerChatApp(unsigned short port) : port_(port), connected_(false), running_(true) {
    // Set static instance for signal handling
    instance_ = this;
    // 시그널 핸들러 설정
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
  }

  void run() {
    // Using convenience function with member function pointers
    auto ul = unilink::tcp_server(port_)
                  .single_client()  // Single client chat
                  .auto_start(false)
                  .enable_port_retry(true, 3, 1000)  // 3회 재시도, 1초 간격
                  .on_connect([this]() { handle_connect(); })
                  .on_disconnect([this]() { handle_disconnect(); })
                  .on_data([this](const std::string& data) { handle_data(data); })
                  .build();

    // Start input thread
    std::thread input_thread([this, &ul] { this->input_loop(ul.get()); });

    // Start the server
    ul->start();

    // Wait for server to start and retry attempts (3 retries * 1s + 0.5s buffer)
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + (3 * 1000)));

    // Check if server started successfully
    if (!ul->is_listening()) {
      unilink::log_message("server", "ERROR", "Failed to start server - port may be in use");
      return;
    }

    // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
    while (running_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    unilink::log_message("server", "INFO", "Shutting down server...");
    input_thread.join();
    ul->stop();
    unilink::log_message("server", "INFO", "Server stopped");
  }

  void stop() { running_ = false; }

  static void signal_handler(int signal) {
    if (instance_ && (signal == SIGINT || signal == SIGTERM)) {
      unilink::log_message("server", "INFO", "Received shutdown signal");
      instance_->stop();
    }
  }

 private:
  // Member function callbacks - these can be bound directly to the builder
  void handle_connect() {
    unilink::log_message("server", "STATE", "Client connected");
    connected_ = true;
  }

  void handle_disconnect() {
    unilink::log_message("server", "STATE", "Client disconnected");
    connected_ = false;
  }

  void handle_data(const std::string& data) { unilink::log_message("server", "RX", data); }

  void input_loop(unilink::wrapper::TcpServer* server) {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (!connected_.load()) {
        unilink::log_message("server", "INFO", "(not connected)");
        continue;
      }
      unilink::log_message("server", "TX", line);
      server->send_line(line);
    }
  }

  unsigned short port_;
  std::atomic<bool> connected_;
  std::atomic<bool> running_;

  static TcpServerChatApp* instance_;
};

// Static member definition
TcpServerChatApp* TcpServerChatApp::instance_ = nullptr;

int main(int argc, char** argv) {
  unsigned short port = (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;

  TcpServerChatApp app(port);
  app.run();

  return 0;
}

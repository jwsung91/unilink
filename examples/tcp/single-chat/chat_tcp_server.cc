#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "unilink/common/logger.hpp"
#include "unilink/unilink.hpp"

/**
 * @brief Chat application class for TCP server
 *
 * This class demonstrates member function binding for TCP server chat functionality.
 */
class TcpServerChatApp {
 private:
  std::shared_ptr<unilink::wrapper::TcpServer> server_;
  unilink::common::Logger& logger_;
  unsigned short port_;
  std::atomic<bool> connected_;
  std::atomic<bool> running_;

 public:
  TcpServerChatApp(unsigned short port)
      : logger_(unilink::common::Logger::instance()), port_(port), connected_(false), running_(true) {
    // Initialize logger
    logger_.set_level(unilink::common::LogLevel::INFO);
    logger_.set_console_output(true);

    // Set static instance for signal handling
    instance_ = this;
    // 시그널 핸들러 설정
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
  }

  ~TcpServerChatApp() {
    // Ensure cleanup in destructor
    if (server_) {
      try {
        server_->stop();
        server_.reset();
      } catch (...) {
        // Ignore errors during destruction
      }
    }
  }

  void signal_handler_instance(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
      if (running_.load()) {
        logger_.info("server", "signal", "Received shutdown signal");
        running_.store(false);
        // Call shutdown immediately to notify clients
        shutdown();
        // Exit immediately after shutdown to prevent double shutdown
        std::exit(0);
      } else {
        // Force exit if already shutting down
        logger_.warning("server", "signal", "Force exit...");
        std::exit(1);
      }
    }
  }

  void run() {
    // Using convenience function with member function pointers
    server_ = unilink::tcp_server(port_)
                  .single_client()  // Single client chat
                  .auto_start(false)
                  .enable_port_retry(true, 3, 1000)  // 3회 재시도, 1초 간격
                  .on_connect([this]() { handle_connect(); })
                  .on_disconnect([this]() { handle_disconnect(); })
                  .on_data([this](const std::string& data) { handle_data(data); })
                  .build();

    if (!server_) {
      logger_.error("server", "startup", "Failed to create server");
      return;
    }

    // Start the server
    server_->start();

    // Wait for server to start and retry attempts (3 retries * 1s + 0.5s buffer)
    std::this_thread::sleep_for(std::chrono::milliseconds(500 + (3 * 1000)));

    // Check if server started successfully
    if (!server_->is_listening()) {
      logger_.error("server", "startup", "Failed to start server - port may be in use");
      return;
    }

    logger_.info("server", "startup", "Server started. Waiting for client connections...");
    print_info();

    // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
    while (running_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      process_input();
    }
  }

  void stop() { running_ = false; }

  static void signal_handler(int signal) {
    if (instance_ && (signal == SIGINT || signal == SIGTERM)) {
      instance_->signal_handler_instance(signal);
    }
  }

  void shutdown() {
    // Cleanup
    logger_.info("server", "shutdown", "Shutting down server...");

    if (server_) {
      // Send shutdown notification to client
      server_->send("[Server] Server is shutting down. Please disconnect.");
      logger_.info("server", "shutdown", "Notified client about shutdown");

      // Give client time to receive message
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // Stop server
      try {
        server_->stop();
        logger_.info("server", "shutdown", "Server stopped");
      } catch (...) {
        logger_.warning("server", "shutdown", "Error stopping server");
      }

      // Clear server pointer to ensure cleanup
      server_.reset();
      logger_.info("server", "shutdown", "Server pointer cleared");

      // Wait for server cleanup
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    logger_.info("server", "shutdown", "Server shutdown complete");
  }

  void print_info() const {
    std::cout << "=== TCP Chat Server (Single Client) ===" << std::endl;
    std::cout << "Port: " << port_ << std::endl;
    std::cout << "Mode: Single client only" << std::endl;
    std::cout << "Exit: Ctrl+C or /quit" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  <message> - Send message to client" << std::endl;
    std::cout << "====================================" << std::endl;
  }

  bool is_running() const { return running_.load(); }

 private:
  // Member function callbacks - these can be bound directly to the builder
  void handle_connect() {
    logger_.info("server", "connect", "Client connected");
    connected_ = true;
  }

  void handle_disconnect() {
    logger_.info("server", "disconnect", "Client disconnected");
    connected_ = false;
  }

  void handle_data(const std::string& data) { logger_.info("server", "data", "Received: " + data); }

  void process_input() {
    // Non-blocking input processing - more robust method
    if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() != EOF) {
      std::string line;
      if (std::getline(std::cin, line)) {
        logger_.info("server", "debug", "Received input: '" + line + "'");
        if (!line.empty()) {
          if (line == "/quit" || line == "/exit") {
            logger_.info("server", "shutdown", "Shutting down server...");
            running_.store(false);
          } else {
            // Send message to client
            if (!connected_.load()) {
              logger_.info("server", "send", "(not connected)");
            } else {
              logger_.info("server", "send", "Sending: " + line);
              if (server_) {
                server_->send_line(line);
              }
            }
          }
        }
      } else {
        // EOF or error on stdin, break the loop
        running_.store(false);
      }
    }
  }

  static TcpServerChatApp* instance_;
};

// Static member definition
TcpServerChatApp* TcpServerChatApp::instance_ = nullptr;

int main(int argc, char** argv) {
  unsigned short port = (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;

  TcpServerChatApp app(port);
  app.run();

  // Force cleanup and exit
  std::cout << "Server process exiting..." << std::endl;
  return 0;
}

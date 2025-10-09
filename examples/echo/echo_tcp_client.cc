#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "unilink/common/logger.hpp"
#include "unilink/unilink.hpp"

class EchoClient {
 private:
  std::shared_ptr<unilink::wrapper::TcpClient> client_;
  unilink::common::Logger& logger_;
  std::atomic<bool> running_;
  std::string server_ip_;
  unsigned short port_;
  std::atomic<bool> auto_send_enabled_;
  std::atomic<uint64_t> message_counter_;

 public:
  EchoClient(const std::string& server_ip, unsigned short port)
      : logger_(unilink::common::Logger::instance()),
        running_(true),
        server_ip_(server_ip),
        port_(port),
        auto_send_enabled_(true),
        message_counter_(0) {
    // Initialize logger
    logger_.set_level(unilink::common::LogLevel::INFO);
    logger_.set_console_output(true);
  }

  ~EchoClient() {
    // Ensure cleanup in destructor
    if (client_) {
      try {
        client_->stop();
        client_.reset();
      } catch (...) {
        // Ignore errors during destruction
      }
    }
  }

  void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
      if (running_.load()) {
        logger_.info("client", "signal", "Received shutdown signal");
        running_.store(false);
        // Force immediate shutdown
        shutdown();
        std::exit(0);
      } else {
        // Force exit if already shutting down
        logger_.warning("client", "signal", "Force exit...");
        std::exit(1);
      }
    }
  }

  void on_connect() {
    logger_.info("client", "connect", "Connected to server " + server_ip_ + ":" + std::to_string(port_));
  }

  void on_data(const std::string& data) {
    logger_.info("client", "data", "Received: " + data);

    // Check for server shutdown notification
    if (data.find("[Server] Server is shutting down") != std::string::npos) {
      logger_.info("client", "shutdown", "Server is shutting down. Disconnecting...");
      running_.store(false);
    }
  }

  void on_disconnect() { logger_.info("client", "disconnect", "Disconnected from server"); }

  void on_error(const std::string& error) { logger_.error("client", "error", "Error: " + error); }

  bool start() {
    // Create TCP client
    client_ = unilink::tcp_client(server_ip_, port_)
                  .on_connect([this]() { on_connect(); })
                  .on_disconnect([this]() { on_disconnect(); })
                  .on_data([this](const std::string& data) { on_data(data); })
                  .on_error([this](const std::string& error) { on_error(error); })
                  .auto_start(true)
                  .retry_interval(3000)  // 3 second retry interval
                  .build();

    if (!client_) {
      logger_.error("client", "startup", "Failed to create client");
      return false;
    }

    // Wait for connection (max 5 seconds)
    bool connected = false;
    for (int i = 0; i < 50; ++i) {
      if (running_.load() == false) {
        logger_.info("client", "startup", "Connection interrupted");
        return false;
      }
      if (client_->is_connected()) {
        connected = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!connected) {
      logger_.error("client", "startup", "Failed to connect to server");
      return false;
    }

    logger_.info("client", "startup", "Client connected successfully");
    return true;
  }

  void auto_send_loop() {
    // Auto-send messages at regular intervals
    uint64_t seq = 0;
    const auto interval = std::chrono::milliseconds(1000);  // 1 second interval

    while (running_.load() && auto_send_enabled_.load()) {
      if (client_ && client_->is_connected()) {
        std::string msg = "ECHO_CLIENT_" + std::to_string(seq++);
        logger_.info("client", "auto_send", "Sending: " + msg);
        client_->send(msg);
        message_counter_.store(seq);
      }
      std::this_thread::sleep_for(interval);
    }
  }

  void process_input() {
    // Non-blocking input processing - more robust method
    if (std::cin.rdbuf()->in_avail() > 0 || std::cin.peek() != EOF) {
      std::string line;
      if (std::getline(std::cin, line)) {
        logger_.info("client", "debug", "Received input: '" + line + "'");
        if (!line.empty()) {
          if (line == "/quit" || line == "/exit") {
            logger_.info("client", "shutdown", "Disconnecting...");
            if (client_) {
              client_->stop();
            }
            running_.store(false);
          } else if (line == "/status") {
            bool connected = client_ ? client_->is_connected() : false;
            logger_.info("client", "status",
                         "Connection status: " + std::string(connected ? "Connected" : "Disconnected"));
            logger_.info("client", "status",
                         "Auto-send enabled: " + std::string(auto_send_enabled_.load() ? "Yes" : "No"));
            logger_.info("client", "status", "Messages sent: " + std::to_string(message_counter_.load()));
          } else if (line == "/auto_on") {
            auto_send_enabled_.store(true);
            logger_.info("client", "auto_send", "Auto-send enabled");
          } else if (line == "/auto_off") {
            auto_send_enabled_.store(false);
            logger_.info("client", "auto_send", "Auto-send disabled");
          } else {
            // Send message to server
            logger_.info("client", "debug", "Sending message to server: '" + line + "'");
            if (client_) {
              client_->send(line);
            }
            logger_.info("client", "send", "Sent: " + line);
          }
        }
      } else {
        // EOF or error on stdin, break the loop
        running_.store(false);
      }
    }
  }

  void run() {
    // Main loop with reconnection logic
    while (running_.load()) {
      logger_.info("client", "connect", "Attempting to connect to server...");

      if (!start()) {
        if (running_.load()) {
          logger_.error("client", "connect", "Failed to connect to server. Retrying in 3 seconds...");
          std::this_thread::sleep_for(std::chrono::seconds(3));
          continue;
        } else {
          break;
        }
      }

      logger_.info("client", "connect", "Connection successful! Auto-sending messages...");

      // Start auto-send thread
      std::thread auto_send_thread([this]() { this->auto_send_loop(); });

      // User input processing loop - don't check is_connected() as it may be unreliable
      while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        process_input();

        // Check if we should exit due to server shutdown
        if (!running_.load()) {
          break;
        }
      }

      // Stop auto-send and wait for thread
      auto_send_enabled_.store(false);
      if (auto_send_thread.joinable()) {
        auto_send_thread.join();
      }

      // If we get here and still running, connection was lost
      if (running_.load()) {
        logger_.info("client", "connect", "Connection lost. Attempting to reconnect...");
        std::this_thread::sleep_for(std::chrono::seconds(3));
      }
    }
  }

  void shutdown() {
    // Cleanup
    logger_.info("client", "shutdown", "Shutting down client...");

    if (client_) {
      try {
        client_->stop();
        logger_.info("client", "shutdown", "Client stopped");
      } catch (...) {
        logger_.warning("client", "shutdown", "Error stopping client");
      }

      // Clear client pointer to ensure cleanup
      client_.reset();
      logger_.info("client", "shutdown", "Client pointer cleared");
    }

    logger_.info("client", "shutdown", "Client shutdown complete");
  }

  void print_info() const {
    std::cout << "=== TCP Echo Client ===" << std::endl;
    std::cout << "Server: " << server_ip_ << ":" << port_ << std::endl;
    std::cout << "Auto-send: 1 second intervals" << std::endl;
    std::cout << "Exit: Ctrl+C or /quit" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  /status - Show connection status" << std::endl;
    std::cout << "  /auto_on - Enable auto-send" << std::endl;
    std::cout << "  /auto_off - Disable auto-send" << std::endl;
    std::cout << "  <message> - Send message to server" << std::endl;
    std::cout << "====================================" << std::endl;
  }

  bool is_running() const { return running_.load(); }
};

// Global pointer for signal handling
EchoClient* g_echo_client = nullptr;

void signal_handler_wrapper(int signal) {
  if (g_echo_client) {
    g_echo_client->signal_handler(signal);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <server_ip> <port>" << std::endl;
    std::cout << "Example: " << argv[0] << " 127.0.0.1 8080" << std::endl;
    return 1;
  }

  std::string server_ip = argv[1];
  unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));

  // Create EchoClient instance
  EchoClient echo_client(server_ip, port);
  g_echo_client = &echo_client;

  // Set up signal handlers
  std::signal(SIGINT, signal_handler_wrapper);
  std::signal(SIGTERM, signal_handler_wrapper);

  echo_client.print_info();

  // Run the main loop
  echo_client.run();

  // Shutdown the client
  echo_client.shutdown();

  // Force cleanup and exit
  std::cout << "Client process exiting..." << std::endl;
  return 0;
}

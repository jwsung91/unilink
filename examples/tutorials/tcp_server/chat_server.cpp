/**
 * @file chat_server.cpp
 * @brief Tutorial 2: Complete chat server example
 *
 * This example demonstrates:
 * - Multi-client chat server
 * - Nickname management
 * - Broadcasting messages
 * - Command handling (/nick, /list, /help)
 *
 * Usage:
 *   ./chat_server [port]
 *
 * Example:
 *   ./chat_server 8080
 *
 * Test with multiple clients:
 *   telnet localhost 8080
 */

#include <atomic>
#include <csignal>
#include <iostream>
#include <map>
#include <mutex>

#include "unilink/unilink.hpp"

// Global flag for graceful shutdown
std::atomic<bool> g_running{true};

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    std::cout << "\nReceived shutdown signal..." << std::endl;
    g_running.store(false);
  }
}

class ChatServer {
 private:
  std::shared_ptr<unilink::wrapper::TcpServer> server_;
  std::map<size_t, std::string> nicknames_;
  std::mutex mutex_;

 public:
  void start(uint16_t port) {
    server_ = unilink::tcp_server(port)
                  .on_connect([this](size_t id, const std::string& ip) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    nicknames_[id] = "User" + std::to_string(id);

                    std::string msg = "*** " + nicknames_[id] + " joined the chat! ***\n";
                    server_->send(msg);
                    std::cout << msg;

                    // Send welcome and help
                    server_->send_to_client(id,
                                            "Welcome to Chat Server!\n"
                                            "Your nickname: " +
                                                nicknames_[id] +
                                                "\n"
                                                "Commands:\n"
                                                "  /nick <name> - Change your nickname\n"
                                                "  /list        - List all users\n"
                                                "  /help        - Show this help\n"
                                                "  /quit        - Disconnect\n"
                                                "\n");
                  })
                  .on_data([this](size_t id, const std::string& data) { handle_message(id, data); })
                  .on_disconnect([this](size_t id) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (nicknames_.count(id)) {
                      std::string msg = "*** " + nicknames_[id] + " left the chat. ***\n";
                      server_->send(msg);
                      std::cout << msg;
                      nicknames_.erase(id);
                    }
                  })
                  .auto_start(true)
                  .build();

    std::cout << "Chat server started on port " << port << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
  }

  void handle_message(size_t id, const std::string& data) {
    // Remove trailing whitespace
    std::string msg = data;
    msg.erase(msg.find_last_not_of(" \n\r\t") + 1);

    if (msg.empty()) return;

    // Handle commands
    if (msg[0] == '/') {
      handle_command(id, msg);
      return;
    }

    // Broadcast message
    std::lock_guard<std::mutex> lock(mutex_);
    std::string broadcast = nicknames_[id] + ": " + msg + "\n";
    server_->send(broadcast);
    std::cout << broadcast;
  }

  void handle_command(size_t id, const std::string& cmd) {
    if (cmd == "/help") {
      server_->send_to_client(id,
                              "Available commands:\n"
                              "  /nick <name> - Change your nickname\n"
                              "  /list        - List all users\n"
                              "  /help        - Show this help\n"
                              "  /quit        - Disconnect\n");
    } else if (cmd == "/list") {
      std::lock_guard<std::mutex> lock(mutex_);
      std::string list = "=== Connected Users ===\n";
      for (const auto& [user_id, nick] : nicknames_) {
        list += "  " + nick;
        if (user_id == id) list += " (you)";
        list += "\n";
      }
      list += "Total: " + std::to_string(nicknames_.size()) + " users\n";
      list += "======================\n";
      server_->send_to_client(id, list);
    } else if (cmd.substr(0, 6) == "/nick ") {
      std::string new_nick = cmd.substr(6);
      if (new_nick.empty()) {
        server_->send_to_client(id, "Usage: /nick <new_name>\n");
        return;
      }

      std::lock_guard<std::mutex> lock(mutex_);
      std::string old_nick = nicknames_[id];
      nicknames_[id] = new_nick;

      std::string msg = "*** " + old_nick + " is now known as " + new_nick + " ***\n";
      server_->send(msg);
      std::cout << msg;
    } else if (cmd == "/quit") {
      server_->send_to_client(id, "Goodbye!\n");
      // Client will disconnect
    } else {
      server_->send_to_client(id, "Unknown command. Type /help for help.\n");
    }
  }

  void stop() {
    if (server_) {
      server_->send("*** Server is shutting down. Goodbye! ***\n");
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      server_->stop();
    }
  }
};

int main(int argc, char** argv) {
  // Set up signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  // Parse port
  uint16_t port = (argc > 1) ? std::stoi(argv[1]) : 8080;

  // Create and start server
  ChatServer server;
  server.start(port);

  // Wait for shutdown signal
  while (g_running.load()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  // Cleanup
  server.stop();
  std::cout << "Chat server stopped." << std::endl;

  return 0;
}

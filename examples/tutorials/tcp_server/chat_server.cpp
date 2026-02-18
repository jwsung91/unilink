/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "unilink/unilink.hpp"

/**
 * Chat Server Tutorial
 * 
 * A more complex example managing client state (nicknames)
 * and using broadcast functionality.
 */

using namespace unilink;

class ChatServer {
 public:
  void start(uint16_t port) {
    server_ = tcp_server(port)
                  .unlimited_clients()
                  .on_connect([this](const wrapper::ConnectionContext& ctx) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    size_t id = ctx.client_id();
                    nicknames_[id] = "User" + std::to_string(id);

                    std::string msg = "*** " + nicknames_[id] + " joined the chat! ***\n";
                    server_->broadcast(msg);
                    std::cout << msg;

                    // Send welcome and help to the new client
                    server_->send_to(id,
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
                  .on_disconnect([this](const wrapper::ConnectionContext& ctx) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    size_t id = ctx.client_id();
                    std::string name = nicknames_[id];
                    nicknames_.erase(id);

                    std::string msg = "*** " + name + " left the chat ***\n";
                    server_->broadcast(msg);
                    std::cout << msg;
                  })
                  .on_data([this](const wrapper::MessageContext& ctx) {
                    std::string data = std::string(ctx.data());
                    if (data.empty()) return;
                    if (data[0] == '/') {
                      handle_command(ctx.client_id(), data);
                    } else {
                      handle_message(ctx.client_id(), data);
                    }
                  })
                  .build();

    if (server_->start().get()) {
      std::cout << "Chat Server started on port " << port << std::endl;
    }
  }

  void handle_message(size_t id, const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string broadcast = "[" + nicknames_[id] + "]: " + data;
    server_->broadcast(broadcast);
  }

  void handle_command(size_t id, const std::string& cmd) {
    if (cmd == "/help") {
      server_->send_to(id, "Help: Use /nick <name>, /list, /quit\n");
    } else if (cmd == "/list") {
      std::lock_guard<std::mutex> lock(mutex_);
      std::string list = "Users: ";
      for (const auto& entry : nicknames_) list += entry.second + " ";
      list += "\n";
      server_->send_to(id, list);
    } else if (cmd.substr(0, 5) == "/nick") {
      if (cmd.length() <= 6) {
        server_->send_to(id, "Usage: /nick <new_name>\n");
        return;
      }
      std::lock_guard<std::mutex> lock(mutex_);
      std::string old_name = nicknames_[id];
      std::string new_name = cmd.substr(6);
      nicknames_[id] = new_name;
      std::string msg = "*** " + old_name + " is now known as " + new_name + " ***\n";
      server_->broadcast(msg);
    } else {
      server_->send_to(id, "Unknown command. Type /help for help.\n");
    }
  }

  void stop() {
    if (server_) {
      server_->broadcast("*** Server is shutting down. Goodbye! ***\n");
      server_->stop();
    }
  }

 private:
  std::shared_ptr<wrapper::TcpServer> server_;
  std::map<size_t, std::string> nicknames_;
  std::mutex mutex_;
};

int main() {
  ChatServer app;
  app.start(8080);
  std::cout << "Press Enter to stop chat server..." << std::endl;
  std::cin.get();
  app.stop();
  return 0;
}

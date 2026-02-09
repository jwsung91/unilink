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

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "unilink/unilink.hpp"

using namespace unilink;

struct ReceiverOptions {
  std::string local_ip = "0.0.0.0";
  uint16_t local_port = 0;
  bool reply = false;
  std::string reply_message = "pong";
};

namespace {

void print_usage(const char* argv0) {
  std::cout << "Usage: " << argv0 << " --local-port <port> [--local-ip <ip>] [--reply]" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --local-port <port>   Local UDP port to bind (required)" << std::endl;
  std::cout << "  --local-ip <ip>       Local address to bind (default: 0.0.0.0)" << std::endl;
  std::cout << "  --reply               Enable replying to the first peer after it is learned" << std::endl;
  std::cout << "  --help                Show this message" << std::endl;
}

bool parse_args(int argc, char** argv, ReceiverOptions& opts, bool& show_help) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--local-port" && i + 1 < argc) {
      opts.local_port = static_cast<uint16_t>(std::stoi(argv[++i]));
    } else if (arg == "--local-ip" && i + 1 < argc) {
      opts.local_ip = argv[++i];
    } else if (arg == "--reply") {
      opts.reply = true;
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      show_help = true;
      return false;
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      print_usage(argv[0]);
      return false;
    }
  }

  if (opts.local_port == 0) {
    std::cerr << "Error: --local-port is required and must be greater than 0." << std::endl;
    print_usage(argv[0]);
    return false;
  }

  return true;
}

}  // namespace

class UdpReceiverApp {
 public:
  explicit UdpReceiverApp(ReceiverOptions opts) : opts_(std::move(opts)), running_(true) {
    instance_ = this;
    std::signal(SIGINT, &UdpReceiverApp::signal_handler);
    std::signal(SIGTERM, &UdpReceiverApp::signal_handler);
  }

  ~UdpReceiverApp() { instance_ = nullptr; }

  int run() {
    try {
      udp_ = unilink::udp(opts_.local_port)
                 .set_local_address(opts_.local_ip)
                 .on_connect([this]() { handle_connect(); })
                 .on_disconnect([this]() { handle_disconnect(); })
                 .on_data([this](const std::string& data) { handle_data(data); })
                 .on_error([this](const std::string& err) { handle_error(err); })
                 .auto_manage(true)
                 .build();
    } catch (const std::exception& ex) {
      std::cerr << "Failed to create UDP receiver: " << ex.what() << std::endl;
      return 1;
    }

    common::log_message("udp-recv", "START", "Listening on " + opts_.local_ip + ":" + std::to_string(opts_.local_port));
    if (opts_.reply) {
      common::log_message("udp-recv", "INFO", "Reply mode enabled (first peer will be remembered)");
    }

    while (running_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (udp_) {
      udp_->stop();
    }
    common::log_message("udp-recv", "STATE", "Receiver stopped");
    return 0;
  }

 private:
  static void signal_handler(int sig) {
    if (instance_ && (sig == SIGINT || sig == SIGTERM)) {
      instance_->running_.store(false);
    }
  }

  void handle_connect() { common::log_message("udp-recv", "STATE", "Peer discovered; replies enabled"); }

  void handle_disconnect() { common::log_message("udp-recv", "STATE", "Disconnected"); }

  void handle_data(const std::string& data) {
    common::log_message("udp-recv", "RX", "Received payload (" + std::to_string(data.size()) + " bytes): " + data);

    if (opts_.reply && udp_ && udp_->is_connected()) {
      udp_->send(opts_.reply_message);
      common::log_message("udp-recv", "TX", "Sent reply: " + opts_.reply_message);
    } else if (opts_.reply) {
      common::log_message("udp-recv", "INFO", "Peer endpoint not ready; reply skipped");
    }
  }

  void handle_error(const std::string& err) { common::log_message("udp-recv", "ERROR", err); }

  ReceiverOptions opts_;
  std::unique_ptr<wrapper::Udp> udp_;
  std::atomic<bool> running_;

  static UdpReceiverApp* instance_;
};

UdpReceiverApp* UdpReceiverApp::instance_ = nullptr;

int main(int argc, char** argv) {
  ReceiverOptions opts;
  bool show_help = false;
  if (!parse_args(argc, argv, opts, show_help)) {
    return show_help ? 0 : 1;
  }

  UdpReceiverApp app(std::move(opts));
  return app.run();
}

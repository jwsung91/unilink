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
#include <limits>
#include <memory>
#include <string>
#include <thread>

#include "unilink/unilink.hpp"

using namespace unilink;

struct SenderOptions {
  std::string remote_ip;
  uint16_t remote_port = 0;
  std::string local_ip = "0.0.0.0";
  uint16_t local_port = 0;
  std::chrono::milliseconds interval{1000};
  uint64_t count = 0;  // 0 = infinite
  std::string message = "ping";
};

namespace {

void print_usage(const char* argv0) {
  std::cout << "Usage: " << argv0
            << " --remote-ip <ip> --remote-port <port> [--local-port <port>] [--local-ip <ip>] "
               "[--interval-ms <ms>] [--count <n>] [--message <text>]"
            << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --remote-ip <ip>      Destination IP (required)" << std::endl;
  std::cout << "  --remote-port <port>  Destination port (required)" << std::endl;
  std::cout << "  --local-port <port>   Local port (default: remote-port + 1)" << std::endl;
  std::cout << "  --local-ip <ip>       Local address (default: 0.0.0.0)" << std::endl;
  std::cout << "  --interval-ms <ms>    Send interval in milliseconds (default: 1000)" << std::endl;
  std::cout << "  --count <n>           Number of messages to send (0 = infinite)" << std::endl;
  std::cout << "  --message <text>      Payload to send (default: \"ping\")" << std::endl;
  std::cout << "  --help                Show this message" << std::endl;
}

bool parse_args(int argc, char** argv, SenderOptions& opts, bool& show_help) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--remote-ip" && i + 1 < argc) {
      opts.remote_ip = argv[++i];
    } else if (arg == "--remote-port" && i + 1 < argc) {
      opts.remote_port = static_cast<uint16_t>(std::stoi(argv[++i]));
    } else if (arg == "--local-port" && i + 1 < argc) {
      opts.local_port = static_cast<uint16_t>(std::stoi(argv[++i]));
    } else if (arg == "--local-ip" && i + 1 < argc) {
      opts.local_ip = argv[++i];
    } else if (arg == "--interval-ms" && i + 1 < argc) {
      opts.interval = std::chrono::milliseconds(std::stoul(argv[++i]));
    } else if (arg == "--count" && i + 1 < argc) {
      opts.count = static_cast<uint64_t>(std::stoull(argv[++i]));
    } else if (arg == "--message" && i + 1 < argc) {
      opts.message = argv[++i];
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

  if (opts.remote_ip.empty() || opts.remote_port == 0) {
    std::cerr << "Error: --remote-ip and --remote-port are required." << std::endl;
    print_usage(argv[0]);
    return false;
  }

  if (opts.local_port == 0) {
    // Default to remote port + 1 to mirror the documented example ports.
    if (opts.remote_port == std::numeric_limits<uint16_t>::max()) {
      opts.local_port = static_cast<uint16_t>(opts.remote_port - 1);
    } else {
      opts.local_port = static_cast<uint16_t>(opts.remote_port + 1);
    }
    if (opts.local_port == 0) {
      opts.local_port = opts.remote_port;
    }
  }

  return true;
}

}  // namespace

class UdpSenderApp {
 public:
  explicit UdpSenderApp(SenderOptions opts) : opts_(std::move(opts)), running_(true), sent_(0) {
    instance_ = this;
    std::signal(SIGINT, &UdpSenderApp::signal_handler);
    std::signal(SIGTERM, &UdpSenderApp::signal_handler);
  }

  ~UdpSenderApp() { instance_ = nullptr; }

  int run() {
    try {
      udp_ = unilink::udp(opts_.local_port)
                 .set_local_address(opts_.local_ip)
                 .set_remote(opts_.remote_ip, opts_.remote_port)
                 .on_connect([this]() { handle_connect(); })
                 .on_disconnect([this]() { handle_disconnect(); })
                 .on_data([this](const std::string& data) { handle_reply(data); })
                 .on_error([this](const std::string& err) { handle_error(err); })
                 .auto_manage(true)
                 .build();
    } catch (const std::exception& ex) {
      std::cerr << "Failed to create UDP sender: " << ex.what() << std::endl;
      return 1;
    }

    common::log_message("udp-send", "START",
                        "Local " + opts_.local_ip + ":" + std::to_string(opts_.local_port) + " -> " + opts_.remote_ip +
                            ":" + std::to_string(opts_.remote_port));

    while (running_.load() && (opts_.count == 0 || sent_.load() < opts_.count)) {
      if (udp_ && udp_->is_connected()) {
        udp_->send(opts_.message);
        sent_.fetch_add(1);
        common::log_message("udp-send", "TX",
                            "Sent: \"" + opts_.message + "\" (count=" + std::to_string(sent_.load()) + ")");
      } else {
        common::log_message("udp-send", "STATE", "Waiting for connection...");
      }
      std::this_thread::sleep_for(opts_.interval);
    }

    if (udp_) {
      udp_->stop();
    }
    common::log_message("udp-send", "STATE", "Sender stopped");
    return 0;
  }

 private:
  static void signal_handler(int sig) {
    if (instance_ && (sig == SIGINT || sig == SIGTERM)) {
      instance_->running_.store(false);
    }
  }

  void handle_connect() { common::log_message("udp-send", "STATE", "Connected (remote endpoint configured)"); }

  void handle_disconnect() { common::log_message("udp-send", "STATE", "Disconnected"); }

  void handle_reply(const std::string& data) {
    common::log_message("udp-send", "RX", "Received reply (" + std::to_string(data.size()) + " bytes): " + data);
  }

  void handle_error(const std::string& err) { common::log_message("udp-send", "ERROR", err); }

  SenderOptions opts_;
  std::shared_ptr<wrapper::Udp> udp_;
  std::atomic<bool> running_;
  std::atomic<uint64_t> sent_;

  static UdpSenderApp* instance_;
};

UdpSenderApp* UdpSenderApp::instance_ = nullptr;

int main(int argc, char** argv) {
  SenderOptions opts;
  bool show_help = false;
  if (!parse_args(argc, argv, opts, show_help)) {
    return show_help ? 0 : 1;
  }

  UdpSenderApp app(std::move(opts));
  return app.run();
}

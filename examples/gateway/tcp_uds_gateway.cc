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

#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include "unilink/unilink.hpp"

/**
 * @brief TCP-to-UDS Gateway Example
 *
 * Demonstrates a multi-protocol gateway pattern:
 *   - A TCP server accepts connections from external clients.
 *   - A UDS client connects to a local backend service.
 *   - Data arriving on TCP is forwarded to the UDS backend.
 *   - Data arriving on UDS is broadcast to all connected TCP clients.
 *
 * Usage:
 *   ./tcp_uds_gateway [tcp_port] [uds_socket_path]
 *
 * Defaults:
 *   tcp_port        = 9000
 *   uds_socket_path = /tmp/unilink_backend.sock
 */

static volatile bool g_running = true;

int main(int argc, char* argv[]) {
  uint16_t tcp_port = 9000;
  std::string uds_path = "/tmp/unilink_backend.sock";

  if (argc > 1) tcp_port = static_cast<uint16_t>(std::stoi(argv[1]));
  if (argc > 2) uds_path = argv[2];

  // Handle Ctrl+C for clean shutdown
  std::signal(SIGINT, [](int) { g_running = false; });
  std::signal(SIGTERM, [](int) { g_running = false; });

  std::cout << "--- TCP-to-UDS Gateway ---\n";
  std::cout << "TCP listen port : " << tcp_port << "\n";
  std::cout << "UDS backend path: " << uds_path << "\n";
  std::cout << "Press Ctrl+C to stop.\n\n";

  // --- TCP Server (external-facing) ---
  std::shared_ptr<unilink::wrapper::TcpServer> tcp_srv;
  std::shared_ptr<unilink::wrapper::UdsClient> uds_cli;

  auto tcp_builder = unilink::tcp_server(tcp_port);
  tcp_builder
      .on_connect([](const unilink::ConnectionContext& ctx) {
        std::cout << "[TCP] Client connected: ID=" << ctx.client_id() << "\n";
      })
      .on_data([&uds_cli](const unilink::MessageContext& ctx) {
        std::cout << "[TCP->UDS] " << ctx.client_id() << ": " << ctx.data() << "\n";
        if (uds_cli && uds_cli->connected()) {
          uds_cli->send(ctx.data());
        } else {
          std::cerr << "[Gateway] UDS backend not connected — message dropped.\n";
        }
      })
      .on_disconnect([](const unilink::ConnectionContext& ctx) {
        std::cout << "[TCP] Client disconnected: ID=" << ctx.client_id() << "\n";
      })
      .on_error([](const unilink::ErrorContext& ctx) {
        std::cerr << "[TCP Error] " << ctx.message() << "\n";
      });

  tcp_srv = tcp_builder.build();

  // --- UDS Client (backend-facing) ---
  auto uds_builder = unilink::uds_client(uds_path);
  uds_builder
      .on_connect([](const unilink::ConnectionContext&) {
        std::cout << "[UDS] Connected to backend service.\n";
      })
      .on_data([&tcp_srv](const unilink::MessageContext& ctx) {
        std::cout << "[UDS->TCP] Broadcasting: " << ctx.data() << "\n";
        if (tcp_srv) {
          tcp_srv->broadcast(ctx.data());
        }
      })
      .on_disconnect([](const unilink::ConnectionContext&) {
        std::cout << "[UDS] Disconnected from backend service.\n";
      })
      .on_error([](const unilink::ErrorContext& ctx) {
        std::cerr << "[UDS Error] " << ctx.message() << "\n";
      });

  uds_cli = uds_builder.build();

  // Start both sides
  tcp_srv->start();
  uds_cli->start();

  std::cout << "Gateway running.\n\n";

  // Main loop — sleep until Ctrl+C
  while (g_running) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if (!g_running) break;
    auto clients = tcp_srv->connected_clients();
    std::cout << "[Status] TCP clients: " << clients.size()
              << " | UDS backend: " << (uds_cli->connected() ? "connected" : "disconnected") << "\n";
  }

  std::cout << "\n[Gateway] Shutting down...\n";
  uds_cli->stop();
  tcp_srv->stop();
  std::cout << "[Gateway] Stopped.\n";

  return 0;
}

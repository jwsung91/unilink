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
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "unilink/factory/channel_factory.hpp"
#include "unilink/config/uds_config.hpp"
#include "unilink/interface/channel.hpp"

using namespace unilink;

int main() {
    config::UdsClientConfig cfg;
    cfg.socket_path = "/tmp/unilink_echo.sock";

    auto client = factory::ChannelFactory::create(cfg);

    client->on_state([](base::LinkState state) {
        std::cout << "Client state changed to: " << base::to_cstr(state) << std::endl;
    });

    client->on_bytes([](memory::ConstByteSpan data) {
        std::string msg(reinterpret_cast<const char*>(data.data()), data.size());
        std::cout << "Received echo: " << msg << std::endl;
    });

    std::cout << "Connecting to UDS server at " << cfg.socket_path << "..." << std::endl;
    client->start();

    // Simple loop to send messages
    std::string input;
    while (true) {
        std::cout << "Enter message (or 'exit' to quit): ";
        std::getline(std::cin, input);
        if (input == "exit") break;

        if (client->is_connected()) {
            client->async_write_copy(memory::ConstByteSpan(reinterpret_cast<const uint8_t*>(input.data()), input.size()));
        } else {
            std::cout << "Client is not connected yet, trying again in 1s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    client->stop();
    return 0;
}

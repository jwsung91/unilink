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

#include <gtest/gtest.h>

#include <chrono>

#include "unilink/builder/uds_builder.hpp"
#include "unilink/framer/line_framer.hpp"

using namespace unilink;
using namespace std::chrono_literals;

namespace unilink {
namespace test {

TEST(UdsBuilderCoverageTest, UdsClientBuilderSetters) {
  auto client = builder::UdsClientBuilder("/tmp/test_uds_client_builder.sock")
                    .auto_start(false)
                    .independent_context(true)
                    .retry_interval(100ms)
                    .max_retries(5)
                    .connection_timeout(1000ms)
                    .on_data([](const wrapper::MessageContext&) {})
                    .on_connect([](const wrapper::ConnectionContext&) {})
                    .on_disconnect([](const wrapper::ConnectionContext&) {})
                    .on_error([](const wrapper::ErrorContext&) {})
                    .framer([]() { return std::make_unique<framer::LineFramer>(); })
                    .on_message([](const wrapper::MessageContext&) {})
                    .build();

  ASSERT_NE(client, nullptr);
}

TEST(UdsBuilderCoverageTest, UdsServerBuilderSetters) {
  auto server = builder::UdsServerBuilder("/tmp/test_uds_server_builder.sock")
                    .auto_start(false)
                    .independent_context(true)
                    .idle_timeout(5000ms)
                    .max_clients(50)
                    .unlimited_clients()
                    .on_data([](const wrapper::MessageContext&) {})
                    .on_connect([](const wrapper::ConnectionContext&) {})
                    .on_disconnect([](const wrapper::ConnectionContext&) {})
                    .on_error([](const wrapper::ErrorContext&) {})
                    .framer([]() { return std::make_unique<framer::LineFramer>(); })
                    .on_message([](const wrapper::MessageContext&) {})
                    .build();

  ASSERT_NE(server, nullptr);
}

}  // namespace test
}  // namespace unilink

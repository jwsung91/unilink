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

#include "unilink/builder/udp_builder.hpp"
#include "unilink/framer/line_framer.hpp"

using namespace unilink;

namespace unilink {
namespace test {

TEST(UdpBuilderCoverageTest, UdpClientBuilderSetters) {
  auto udp = builder::UdpClientBuilder(0)
                 .auto_start(false)  // Disable auto-start for stability in builder tests
                 .local_port(0)
                 .remote("127.0.0.1", 5678)
                 .independent_context(true)
                 .broadcast(true)
                 .reuse_address(true)
                 .on_data([](const wrapper::MessageContext&) {})
                 .on_connect([](const wrapper::ConnectionContext&) {})
                 .on_disconnect([](const wrapper::ConnectionContext&) {})
                 .on_error([](const wrapper::ErrorContext&) {})
                 .framer([]() { return std::make_unique<framer::LineFramer>(); })
                 .on_message([](const wrapper::MessageContext&) {})
                 .build();

  ASSERT_NE(udp, nullptr);
}

TEST(UdpBuilderCoverageTest, UdpServerBuilderSetters) {
  auto server = builder::UdpServerBuilder(0)
                    .auto_start(false)
                    .local_port(0)
                    .independent_context(true)
                    .broadcast(true)
                    .reuse_address(true)
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

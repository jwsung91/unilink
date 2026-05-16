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
#include <vector>

#include "unilink/builder/udp_builder.hpp"
#include "unilink/diagnostics/exceptions.hpp"
#include "unilink/framer/line_framer.hpp"

using namespace unilink;
using namespace std::chrono_literals;

namespace unilink {
namespace test {

TEST(UdpBuilderOptionsTest, UdpClientBuilderSetters) {
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
                 .on_data([](auto&&) {})
                 .on_error([](auto&&) {})
                 .build();

  ASSERT_NE(udp, nullptr);
}

TEST(UdpBuilderOptionsTest, UdpServerBuilderSetters) {
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
                    .on_data([](auto&&) {})
                    .on_error([](auto&&) {})
                    .build();

  ASSERT_NE(server, nullptr);
}

TEST(UdpBuilderOptionsTest, UdpClientBuilderSettersAfterDataHandler) {
  auto builder = builder::UdpClientBuilder<>(0).on_data_batch([](const std::vector<wrapper::MessageContext>&) {});

  auto udp = std::move(builder)
                 .auto_start(false)
                 .local_port(0)
                 .bind_address("127.0.0.1")
                 .remote_endpoint("127.0.0.1", 5678)
                 .broadcast(false)
                 .reuse_address(false)
                 .independent_context(false)
                 .backpressure_strategy(base::constants::BackpressureStrategy::BestEffort)
                 .backpressure_threshold(2048)
                 .on_backpressure([](size_t) {})
                 .use_packet_framer(std::vector<uint8_t>{0x02}, std::vector<uint8_t>{0x03}, 128)
                 .on_connect([](const wrapper::ConnectionContext&) {})
                 .on_disconnect([](const wrapper::ConnectionContext&) {})
                 .on_message([](const wrapper::MessageContext&) {})
                 .on_message_batch([](const std::vector<wrapper::MessageContext>&) {})
                 .on_error([](const wrapper::ErrorContext&) {})
                 .build();

  ASSERT_NE(udp, nullptr);
}

TEST(UdpBuilderOptionsTest, UdpClientBuilderSettersAfterErrorHandler) {
  auto builder = builder::UdpClientBuilder<>(0).on_error([](const wrapper::ErrorContext&) {});

  auto udp = std::move(builder)
                 .auto_start(false)
                 .local_port(0)
                 .bind_address("0.0.0.0")
                 .remote("127.0.0.1", 5678)
                 .broadcast(true)
                 .reuse_address(true)
                 .independent_context(true)
                 .backpressure_strategy(base::constants::BackpressureStrategy::Reliable)
                 .backpressure_threshold(4096)
                 .on_backpressure([](size_t) {})
                 .use_line_framer("\n", false, 128)
                 .on_connect([](const wrapper::ConnectionContext&) {})
                 .on_disconnect([](const wrapper::ConnectionContext&) {})
                 .on_data([](const wrapper::MessageContext&) {})
                 .build();

  ASSERT_NE(udp, nullptr);
}

TEST(UdpBuilderOptionsTest, UdpServerBuilderSettersAfterDataHandler) {
  auto builder = builder::UdpServerBuilder<>(0).on_data_batch([](const std::vector<wrapper::MessageContext>&) {});

  auto server = std::move(builder)
                    .auto_start(false)
                    .local_port(0)
                    .bind_address("127.0.0.1")
                    .max_clients(2)
                    .idle_timeout(25ms)
                    .broadcast(false)
                    .reuse_address(false)
                    .independent_context(false)
                    .backpressure_strategy(base::constants::BackpressureStrategy::BestEffort)
                    .backpressure_threshold(2048)
                    .on_backpressure([](size_t) {})
                    .use_packet_framer(std::vector<uint8_t>{0x02}, std::vector<uint8_t>{0x03}, 128)
                    .on_connect([](const wrapper::ConnectionContext&) {})
                    .on_disconnect([](const wrapper::ConnectionContext&) {})
                    .on_message([](const wrapper::MessageContext&) {})
                    .on_message_batch([](const std::vector<wrapper::MessageContext>&) {})
                    .on_error([](const wrapper::ErrorContext&) {})
                    .build();

  ASSERT_NE(server, nullptr);
}

TEST(UdpBuilderOptionsTest, UdpServerBuilderSettersAfterErrorHandler) {
  auto builder = builder::UdpServerBuilder<>(0).on_error([](const wrapper::ErrorContext&) {});

  auto server = std::move(builder)
                    .auto_start(false)
                    .local_port(0)
                    .bind_address("0.0.0.0")
                    .max_clients(3)
                    .idle_timeout(50ms)
                    .broadcast(true)
                    .reuse_address(true)
                    .independent_context(true)
                    .backpressure_strategy(base::constants::BackpressureStrategy::Reliable)
                    .backpressure_threshold(4096)
                    .on_backpressure([](size_t) {})
                    .use_line_framer("\n", false, 128)
                    .on_connect([](const wrapper::ConnectionContext&) {})
                    .on_disconnect([](const wrapper::ConnectionContext&) {})
                    .on_data([](const wrapper::MessageContext&) {})
                    .build();

  ASSERT_NE(server, nullptr);
}

TEST(UdpBuilderOptionsTest, MissingMandatoryHandlersThrow) {
#if __cplusplus >= 202002L
  EXPECT_THROW(builder::UdpClientBuilder<>(0).build(), diagnostics::BuilderException);
  EXPECT_THROW(builder::UdpServerBuilder<>(0).build(), diagnostics::BuilderException);
#endif
}

}  // namespace test
}  // namespace unilink

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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <memory>

#include "test_utils.hpp"
#include "unilink/builder/udp_builder.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/wrapper/udp/udp.hpp"

using namespace unilink::wrapper;
using namespace unilink::config;
using namespace unilink::test;

class UdpOptionsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Basic setup
  }

  void TearDown() override {
    // Basic teardown
  }
};

TEST_F(UdpOptionsTest, SetterCoverage) {
  UdpConfig config;
  config.local_port = 0;

  Udp udp(config);

  // Test auto_manage setter
  // It returns ChannelInterface& so we can chain it, but Udp wrapper implements it.
  udp.auto_manage(true);
  udp.auto_manage(false);

  // Test set_manage_external_context
  udp.set_manage_external_context(true);
  udp.set_manage_external_context(false);
}

TEST_F(UdpOptionsTest, ConstructorWithExternalContext) {
  UdpConfig config;
  config.local_port = 0;

  auto ioc = std::make_shared<boost::asio::io_context>();
  Udp udp(config, ioc);

  // Should not throw
  udp.auto_manage(false);
}

TEST_F(UdpOptionsTest, BuilderCoverageForUdpSocketOptions) {
  unilink::builder::UdpClientBuilder client_builder;
  client_builder.enable_broadcast(true).reuse_address(true);

  unilink::builder::UdpServerBuilder server_builder;
  server_builder.enable_broadcast(true).reuse_address(true);
}

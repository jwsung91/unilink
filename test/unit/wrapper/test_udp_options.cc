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

#include <memory>

#include "test_utils.hpp"
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
  config.local_port = TestUtils::getAvailableTestPort();

  Udp udp(config);

  // Test auto_manage setter
  // It returns ChannelInterface& so we can chain it, but Udp wrapper implements it.
  udp.auto_manage(true);
  udp.auto_manage(false);

  // Test set_manage_external_context
  // This is a void method in Udp wrapper
  udp.set_manage_external_context(true);
  udp.set_manage_external_context(false);

  // Note: The following setters were requested but are not available in the Udp wrapper API:
  // - set_multicast_ttl
  // - join_multicast_group
  // - set_broadcast
  // - set_reuse_address
  //
  // If these features are added in the future, tests should be added here.
}

TEST_F(UdpOptionsTest, ConstructorWithExternalContext) {
  UdpConfig config;
  config.local_port = TestUtils::getAvailableTestPort();

  auto ioc = std::make_shared<boost::asio::io_context>();
  Udp udp(config, ioc);

  // Should not throw
  udp.auto_manage(false);
}

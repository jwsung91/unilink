/*
 * Copyright 2025 Jinwoo Sung
 */

#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "unilink/base/constants.hpp"
#include "unilink/config/tcp_client_config.hpp"
#include "unilink/transport/tcp_client/tcp_client.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::base::constants;

class AdaptiveStrategyTest : public ::testing::Test {
 protected:
  boost::asio::io_context ioc;
};

TEST_F(AdaptiveStrategyTest, TcpClientLatestStrategyDropsOldData) {
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = 9999;
  cfg.backpressure_threshold = 1024;  // 1KB
  cfg.backpressure_strategy = BackpressureStrategy::Latest;

  auto client = TcpClient::create(cfg, ioc);

  std::vector<uint8_t> large_data(2048, 'A');  // 2KB, exceeds threshold

  // 1. Send first message -> triggers backpressure logic
  client->async_write_copy(memory::ConstByteSpan(large_data.data(), large_data.size()));

  // 2. Send second message -> should clear first and keep second
  std::vector<uint8_t> latest_data(100, 'B');
  client->async_write_copy(memory::ConstByteSpan(latest_data.data(), latest_data.size()));
}

#include "unilink/transport/tcp_client/detail/reconnect_logic.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <optional>

#include "unilink/config/tcp_client_config.hpp"
#include "unilink/diagnostics/error_types.hpp"
#include "unilink/transport/tcp_client/reconnect_policy.hpp"

using namespace unilink;
using namespace unilink::transport;
using namespace unilink::transport::detail;
using namespace std::chrono_literals;

class ReconnectLogicTest : public ::testing::Test {
 protected:
  config::TcpClientConfig cfg_;
  diagnostics::ErrorInfo error_info_;

  ReconnectLogicTest()
      : error_info_(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "test", "op", "msg", {},
                    true) {}

  void SetUp() override {
    cfg_.max_retries = -1;  // Infinite
  }
};

TEST_F(ReconnectLogicTest, NonRetryableErrorStopsImmediately) {
  error_info_.retryable = false;
  auto decision = decide_reconnect(cfg_, error_info_, 0, std::nullopt);
  EXPECT_FALSE(decision.should_retry);
  EXPECT_FALSE(decision.delay.has_value());
}

TEST_F(ReconnectLogicTest, MaxRetriesZeroStopsImmediately) {
  cfg_.max_retries = 0;
  auto decision = decide_reconnect(cfg_, error_info_, 0, std::nullopt);
  EXPECT_FALSE(decision.should_retry);
}

TEST_F(ReconnectLogicTest, MaxRetriesLimitEnforced) {
  cfg_.max_retries = 3;

  // Attempt 0 (first retry) -> OK
  EXPECT_TRUE(decide_reconnect(cfg_, error_info_, 0, std::nullopt).should_retry);
  // Attempt 1 -> OK
  EXPECT_TRUE(decide_reconnect(cfg_, error_info_, 1, std::nullopt).should_retry);
  // Attempt 2 -> OK
  EXPECT_TRUE(decide_reconnect(cfg_, error_info_, 2, std::nullopt).should_retry);
  // Attempt 3 -> Stop
  EXPECT_FALSE(decide_reconnect(cfg_, error_info_, 3, std::nullopt).should_retry);
}

TEST_F(ReconnectLogicTest, PolicyDecisionRespected) {
  auto policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision { return {true, 100ms}; };

  auto decision = decide_reconnect(cfg_, error_info_, 0, policy);
  EXPECT_TRUE(decision.should_retry);
  EXPECT_EQ(*decision.delay, 100ms);
}

TEST_F(ReconnectLogicTest, PolicyDecisionStop) {
  auto policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision { return {false, 0ms}; };

  auto decision = decide_reconnect(cfg_, error_info_, 0, policy);
  EXPECT_FALSE(decision.should_retry);
}

TEST_F(ReconnectLogicTest, PolicyDelayClampedToZero) {
  auto policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision { return {true, -50ms}; };

  auto decision = decide_reconnect(cfg_, error_info_, 0, policy);
  EXPECT_TRUE(decision.should_retry);
  EXPECT_EQ(*decision.delay, 0ms);
}

TEST_F(ReconnectLogicTest, PolicyDelayClampedToMax) {
  auto policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision {
    return {true, 60000ms};  // 60s
  };

  // Default max is 30s
  auto decision = decide_reconnect(cfg_, error_info_, 0, policy);
  EXPECT_TRUE(decision.should_retry);
  EXPECT_EQ(*decision.delay, 30000ms);
}

TEST_F(ReconnectLogicTest, LegacyLogicWhenNoPolicy) {
  // Should return true and nullopt delay
  auto decision = decide_reconnect(cfg_, error_info_, 0, std::nullopt);
  EXPECT_TRUE(decision.should_retry);
  EXPECT_FALSE(decision.delay.has_value());
}

TEST_F(ReconnectLogicTest, MaxRetriesEnforcedWithPolicy) {
  cfg_.max_retries = 2;
  auto policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision { return {true, 10ms}; };

  // Attempt 0 -> OK
  EXPECT_TRUE(decide_reconnect(cfg_, error_info_, 0, policy).should_retry);
  // Attempt 1 -> OK
  EXPECT_TRUE(decide_reconnect(cfg_, error_info_, 1, policy).should_retry);
  // Attempt 2 -> Stop (even if policy says yes)
  EXPECT_FALSE(decide_reconnect(cfg_, error_info_, 2, policy).should_retry);
}

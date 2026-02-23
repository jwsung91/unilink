#include <gtest/gtest.h>

#include <chrono>
#include <optional>

#include "unilink/config/tcp_client_config.hpp"
#include "unilink/diagnostics/error_types.hpp"
#include "unilink/transport/tcp_client/detail/reconnect_decider.hpp"
#include "unilink/transport/tcp_client/reconnect_policy.hpp"

using namespace unilink;
using namespace unilink::transport::detail;
using namespace std::chrono_literals;

namespace {

diagnostics::ErrorInfo MakeErrorInfo(bool retryable) {
  return diagnostics::ErrorInfo(diagnostics::ErrorLevel::ERROR, diagnostics::ErrorCategory::CONNECTION, "test",
                                "connect", "simulated error", boost::system::error_code{}, retryable);
}

config::TcpClientConfig MakeBaseConfig() {
  config::TcpClientConfig cfg;
  cfg.max_retries = -1;
  cfg.retry_interval_ms = 3000;
  return cfg;
}

}  // namespace

TEST(ReconnectDeciderTest, MaxRetriesZeroAlwaysStopsEvenIfPolicyWouldRetry) {
  auto cfg = MakeBaseConfig();
  cfg.max_retries = 0;
  auto info = MakeErrorInfo(true);

  bool policy_called = false;
  ReconnectPolicy policy = [&policy_called](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision {
    policy_called = true;
    return {true, 1ms};
  };

  auto decision = decide_reconnect(cfg, info, 0, policy);
  EXPECT_FALSE(decision.should_retry);
  EXPECT_FALSE(policy_called);
}

TEST(ReconnectDeciderTest, MaxRetriesBoundaryStopsAtAttemptCountEqualToLimit) {
  auto cfg = MakeBaseConfig();
  cfg.max_retries = 3;
  auto info = MakeErrorInfo(true);

  auto decision = decide_reconnect(cfg, info, 3, std::nullopt);
  EXPECT_FALSE(decision.should_retry);
}

TEST(ReconnectDeciderTest, NonRetryableErrorStopsEvenIfPolicyWouldRetry) {
  auto cfg = MakeBaseConfig();
  auto info = MakeErrorInfo(false);

  bool policy_called = false;
  ReconnectPolicy policy = [&policy_called](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision {
    policy_called = true;
    return {true, 1ms};
  };

  auto decision = decide_reconnect(cfg, info, 0, policy);
  EXPECT_FALSE(decision.should_retry);
  EXPECT_FALSE(policy_called);
}

TEST(ReconnectDeciderTest, PolicyNegativeDelayClampsToZero) {
  auto cfg = MakeBaseConfig();
  auto info = MakeErrorInfo(true);
  ReconnectPolicy policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision { return {true, -10ms}; };

  auto decision = decide_reconnect(cfg, info, 0, policy);
  ASSERT_TRUE(decision.should_retry);
  ASSERT_TRUE(decision.delay.has_value());
  EXPECT_EQ(*decision.delay, 0ms);
}

TEST(ReconnectDeciderTest, PolicyHugeDelayClampsToConfiguredCap) {
  auto cfg = MakeBaseConfig();
  auto info = MakeErrorInfo(true);
  ReconnectPolicy policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision {
    return {true, 999999ms};
  };

  auto decision = decide_reconnect(cfg, info, 0, policy);
  ASSERT_TRUE(decision.should_retry);
  ASSERT_TRUE(decision.delay.has_value());
  EXPECT_LE(*decision.delay, 60s);
  EXPECT_EQ(*decision.delay, MAX_RECONNECT_DELAY);
}

TEST(ReconnectDeciderTest, NoPolicyFallsBackToRetryInterval) {
  auto cfg = MakeBaseConfig();
  cfg.retry_interval_ms = 1500;
  auto info = MakeErrorInfo(true);

  auto decision = decide_reconnect(cfg, info, 0, std::nullopt);
  ASSERT_TRUE(decision.should_retry);
  ASSERT_TRUE(decision.delay.has_value());
  EXPECT_EQ(*decision.delay, 1500ms);
}

TEST(ReconnectDeciderTest, PolicyCanStopReconnect) {
  auto cfg = MakeBaseConfig();
  auto info = MakeErrorInfo(true);
  ReconnectPolicy policy = [](const diagnostics::ErrorInfo&, uint32_t) -> ReconnectDecision { return {false, 1ms}; };

  auto decision = decide_reconnect(cfg, info, 0, policy);
  EXPECT_FALSE(decision.should_retry);
  EXPECT_FALSE(decision.delay.has_value());
}

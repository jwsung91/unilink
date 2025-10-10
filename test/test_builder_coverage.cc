#include <gtest/gtest.h>

#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;

/**
 * @brief Builder coverage tests to improve code coverage
 * 
 * These tests exercise all builder methods to ensure they are tested
 */
class BuilderCoverageTest : public BaseTest {
protected:
  void SetUp() override {
    BaseTest::SetUp();
    test_port_ = TestUtils::getAvailableTestPort();
  }

  uint16_t test_port_;
};

// ============================================================================
// TCP SERVER BUILDER COVERAGE
// ============================================================================

TEST_F(BuilderCoverageTest, TcpServerBuilderAllMethods) {
  // Test all TcpServerBuilder methods
  auto server = tcp_server(test_port_)
                    .auto_start(false)
                    .auto_manage(false)
                    .unlimited_clients()
                    .on_connect([]() { /* connect callback */ })
                    .on_disconnect([]() { /* disconnect callback */ })
                    .on_data([](const std::string& data) { /* data callback */ })
                    .on_error([](const std::string& error) { /* error callback */ })
                    .enable_port_retry(true, 3, 1000)
                    .build();

  EXPECT_NE(server, nullptr);
}

TEST_F(BuilderCoverageTest, TcpServerBuilderSingleClient) {
  auto server = tcp_server(test_port_)
                    .single_client()
                    .auto_start(false)
                    .build();

  EXPECT_NE(server, nullptr);
}

TEST_F(BuilderCoverageTest, TcpServerBuilderMultiClient) {
  auto server = tcp_server(test_port_)
                    .multi_client(10)
                    .on_multi_connect([](size_t client_id, const std::string& ip) { /* multi connect */ })
                    .on_multi_data([](size_t client_id, const std::string& data) { /* multi data */ })
                    .on_multi_disconnect([](size_t client_id) { /* multi disconnect */ })
                    .build();

  EXPECT_NE(server, nullptr);
}

// ============================================================================
// TCP CLIENT BUILDER COVERAGE
// ============================================================================

TEST_F(BuilderCoverageTest, TcpClientBuilderAllMethods) {
  // Test all TcpClientBuilder methods
  auto client = tcp_client("127.0.0.1", test_port_)
                    .auto_start(false)
                    .auto_manage(false)
                    .on_connect([]() { /* connect callback */ })
                    .on_disconnect([]() { /* disconnect callback */ })
                    .on_data([](const std::string& data) { /* data callback */ })
                    .on_error([](const std::string& error) { /* error callback */ })
                    .build();

  EXPECT_NE(client, nullptr);
}

TEST_F(BuilderCoverageTest, TcpClientBuilderWithAutoManage) {
  auto client = tcp_client("127.0.0.1", test_port_)
                    .auto_manage(true)
                    .auto_start(false)
                    .build();

  EXPECT_NE(client, nullptr);
}

// ============================================================================
// SERIAL BUILDER COVERAGE
// ============================================================================

TEST_F(BuilderCoverageTest, SerialBuilderAllMethods) {
  // Test all SerialBuilder methods
  auto serial_port = serial("/dev/ttyUSB0", 115200)
                         .auto_start(false)
                         .auto_manage(false)
                         .on_connect([]() { /* connect callback */ })
                         .on_disconnect([]() { /* disconnect callback */ })
                         .on_data([](const std::string& data) { /* data callback */ })
                         .on_error([](const std::string& error) { /* error callback */ })
                         .build();

  // Serial device might not exist, but builder should work
  EXPECT_NE(serial_port, nullptr);
}

TEST_F(BuilderCoverageTest, SerialBuilderWithAutoManage) {
  auto serial_port = serial("/dev/ttyUSB0", 9600)
                         .auto_manage(true)
                         .auto_start(false)
                         .build();

  EXPECT_NE(serial_port, nullptr);
}

// ============================================================================
// UNIFIED BUILDER COVERAGE
// ============================================================================

TEST_F(BuilderCoverageTest, UnifiedBuilderTcpServer) {
  auto server = builder::UnifiedBuilder::tcp_server(test_port_)
                    .unlimited_clients()
                    .auto_start(false)
                    .build();

  EXPECT_NE(server, nullptr);
}

TEST_F(BuilderCoverageTest, UnifiedBuilderTcpClient) {
  auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port_)
                    .auto_start(false)
                    .build();

  EXPECT_NE(client, nullptr);
}

TEST_F(BuilderCoverageTest, UnifiedBuilderSerial) {
  auto serial_port = builder::UnifiedBuilder::serial("/dev/ttyUSB0", 115200)
                         .auto_start(false)
                         .build();

  EXPECT_NE(serial_port, nullptr);
}

// ============================================================================
// BUILDER VALIDATION
// ============================================================================

TEST_F(BuilderCoverageTest, TcpServerBuilderInvalidClientLimit) {
  // Client limit of 1 should throw
  EXPECT_THROW(tcp_server(test_port_).max_clients(1).build(), 
               std::invalid_argument);
}

TEST_F(BuilderCoverageTest, TcpServerBuilderZeroClientLimit) {
  // Client limit of 0 means unlimited
  auto server = tcp_server(test_port_)
                    .max_clients(0)
                    .auto_start(false)
                    .build();

  EXPECT_NE(server, nullptr);
}


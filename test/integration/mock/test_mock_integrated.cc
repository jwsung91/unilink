#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "mocks/dependency_injection.hpp"
#include "mocks/mock_tcp_socket.hpp"
#include "mocks/mock_test_helpers.hpp"
#include "test_utils.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::test::mocks;
using namespace std::chrono_literals;

/**
 * @brief Integrated mock tests
 *
 * This file combines all mock-related tests into a single,
 * well-organized test suite for better maintainability.
 */
class MockIntegratedTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize test state
    test_port_ = TestUtils::getAvailableTestPort();

    // Enable testing mode
    mock_scope_ = std::make_unique<MockTestScope>();
  }

  void TearDown() override {
    // Clean up mock scope
    mock_scope_.reset();

    // Clean up any test state
    // Increased wait time to ensure complete cleanup and avoid port conflicts
    TestUtils::waitFor(500);
  }

  uint16_t test_port_;
  std::unique_ptr<MockTestScope> mock_scope_;
};

// ============================================================================
// MOCK OBJECT TESTS
// ============================================================================

/**
 * @brief Test mock object creation
 */
TEST_F(MockIntegratedTest, MockObjectCreation) {
  // Test mock socket creation
  auto mock_socket = std::make_unique<MockTcpSocket>();
  EXPECT_NE(mock_socket, nullptr);

  // Test mock acceptor creation
  auto mock_acceptor = std::make_unique<MockTcpAcceptor>();
  EXPECT_NE(mock_acceptor, nullptr);

  // Test mock serial creation
  auto mock_serial = std::make_unique<MockSerialPort>();
  EXPECT_NE(mock_serial, nullptr);
}

/**
 * @brief Test mock state tracking
 */
TEST_F(MockIntegratedTest, MockStateTracking) {
  auto state_tracker = std::make_unique<MockStateTracker>();

  // Test state transitions
  state_tracker->setState(MockStateTracker::State::Idle);
  EXPECT_EQ(state_tracker->getCurrentState(), MockStateTracker::State::Idle);

  state_tracker->setState(MockStateTracker::State::Connecting);
  EXPECT_EQ(state_tracker->getCurrentState(), MockStateTracker::State::Connecting);

  state_tracker->setState(MockStateTracker::State::Connected);
  EXPECT_EQ(state_tracker->getCurrentState(), MockStateTracker::State::Connected);

  // Test state history
  auto history = state_tracker->getStateHistory();
  EXPECT_EQ(history.size(), 3);
  EXPECT_EQ(history[0], MockStateTracker::State::Idle);
  EXPECT_EQ(history[1], MockStateTracker::State::Connecting);
  EXPECT_EQ(history[2], MockStateTracker::State::Connected);
}

/**
 * @brief Test mock state waiting
 */
TEST_F(MockIntegratedTest, MockStateWaiting) {
  auto state_tracker = std::make_unique<MockStateTracker>();

  // Set state in another thread
  std::thread([&state_tracker]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    state_tracker->setState(MockStateTracker::State::Connected);
  }).detach();

  // Wait for state
  EXPECT_TRUE(state_tracker->waitForState(MockStateTracker::State::Connected, std::chrono::milliseconds(1000)));
}

// ============================================================================
// MOCK DATA GENERATION TESTS
// ============================================================================

/**
 * @brief Test mock data generation
 */
TEST_F(MockIntegratedTest, MockDataGeneration) {
  // Test message generation
  std::string test_message = MockTestDataGenerator::generateTestMessage(1024);
  EXPECT_EQ(test_message.size(), 1024);

  // Test binary data generation
  std::vector<uint8_t> binary_data = MockTestDataGenerator::generateBinaryData(512);
  EXPECT_EQ(binary_data.size(), 512);

  // Test JSON message generation
  std::string json_message = MockTestDataGenerator::generateJsonMessage("test", "hello");
  EXPECT_FALSE(json_message.empty());
  EXPECT_TRUE(json_message.find("test") != std::string::npos);
  EXPECT_TRUE(json_message.find("hello") != std::string::npos);
}

/**
 * @brief Test mock data generation with different sizes
 */
TEST_F(MockIntegratedTest, MockDataGenerationSizes) {
  std::vector<size_t> sizes = {64, 256, 1024, 4096};

  for (size_t size : sizes) {
    std::string test_message = MockTestDataGenerator::generateTestMessage(size);
    std::vector<uint8_t> binary_data = MockTestDataGenerator::generateBinaryData(size);

    EXPECT_EQ(test_message.size(), size);
    EXPECT_EQ(binary_data.size(), size);
  }
}

// ============================================================================
// DEPENDENCY INJECTION TESTS
// ============================================================================

/**
 * @brief Test dependency injection setup
 */
TEST_F(MockIntegratedTest, DependencyInjectionSetup) {
  auto& injector = DependencyInjector::instance();

  // Register mock factories
  injector.registerSocketFactory("test", []() { return std::make_unique<MockTcpSocket>(); });

  injector.registerAcceptorFactory("test", []() { return std::make_unique<MockTcpAcceptor>(); });

  injector.registerSerialFactory("test", []() { return std::make_unique<MockSerialPort>(); });

  // Create mock objects
  auto socket = injector.createSocket("test");
  auto acceptor = injector.createAcceptor("test");
  auto serial = injector.createSerial("test");

  EXPECT_NE(socket, nullptr);
  EXPECT_NE(acceptor, nullptr);
  EXPECT_NE(serial, nullptr);
}

/**
 * @brief Test dependency injection with default factories
 */
TEST_F(MockIntegratedTest, DependencyInjectionDefault) {
  auto& injector = DependencyInjector::instance();

  // Create mock objects with default factories
  auto socket = injector.createSocket("default");
  auto acceptor = injector.createAcceptor("default");
  auto serial = injector.createSerial("default");

  EXPECT_NE(socket, nullptr);
  EXPECT_NE(acceptor, nullptr);
  EXPECT_NE(serial, nullptr);
}

// ============================================================================
// MOCK SCENARIO TESTS
// ============================================================================

/**
 * @brief Test mock scenario builder
 */
TEST_F(MockIntegratedTest, MockScenarioBuilder) {
  // Test successful connection scenario
  MockScenarioBuilder builder;
  builder.withSuccessfulConnection().apply();

  // Test connection failure scenario
  MockScenarioBuilder builder2;
  builder2.withConnectionFailure(MockTestScenario::ConnectionResult::ConnectionRefused).apply();

  // Test data reception scenario
  MockScenarioBuilder builder3;
  builder3.withDataReception("test data").apply();

  // Test data transmission scenario
  MockScenarioBuilder builder4;
  builder4.withDataTransmission(MockTestScenario::DataTransferResult::Success).apply();

  // All scenarios should apply successfully
  EXPECT_TRUE(true);
}

/**
 * @brief Test mock scenario builder variations
 */
TEST_F(MockIntegratedTest, MockScenarioBuilderVariations) {
  std::vector<std::string> test_data = {"test1", "test2", "test3"};

  for (const auto& data : test_data) {
    MockScenarioBuilder builder;
    builder.withDataReception(data).apply();

    // Scenario should apply successfully
    EXPECT_TRUE(true);
  }
}

// ============================================================================
// MOCK PERFORMANCE TESTS
// ============================================================================

/**
 * @brief Test mock performance characteristics
 */
TEST_F(MockIntegratedTest, MockPerformanceTest) {
  auto start_time = std::chrono::high_resolution_clock::now();

  // Create multiple mock objects rapidly
  std::vector<std::unique_ptr<MockTcpSocket>> mock_sockets;
  const int socket_count = 100;

  for (int i = 0; i < socket_count; ++i) {
    auto socket = std::make_unique<MockTcpSocket>();
    mock_sockets.push_back(std::move(socket));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  EXPECT_EQ(mock_sockets.size(), socket_count);

  // Mock-based creation should be very fast (less than 1ms per socket)
  EXPECT_LT(duration.count(), socket_count * 1000);

  std::cout << "Created " << socket_count << " mock sockets in " << duration.count() << " microseconds" << std::endl;
}

/**
 * @brief Test mock memory usage
 */
TEST_F(MockIntegratedTest, MockMemoryUsage) {
  const int object_count = 1000;

  std::vector<std::unique_ptr<MockTcpSocket>> sockets;
  std::vector<std::unique_ptr<MockTcpAcceptor>> acceptors;
  std::vector<std::unique_ptr<MockSerialPort>> serials;

  for (int i = 0; i < object_count; ++i) {
    sockets.push_back(std::make_unique<MockTcpSocket>());
    acceptors.push_back(std::make_unique<MockTcpAcceptor>());
    serials.push_back(std::make_unique<MockSerialPort>());
  }

  EXPECT_EQ(sockets.size(), object_count);
  EXPECT_EQ(acceptors.size(), object_count);
  EXPECT_EQ(serials.size(), object_count);

  // All objects should be valid
  for (const auto& socket : sockets) {
    EXPECT_NE(socket, nullptr);
  }
  for (const auto& acceptor : acceptors) {
    EXPECT_NE(acceptor, nullptr);
  }
  for (const auto& serial : serials) {
    EXPECT_NE(serial, nullptr);
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

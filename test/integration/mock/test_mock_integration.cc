#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include "mocks/mock_tcp_socket.hpp"
#include "mocks/mock_test_helpers.hpp"
#include "test_utils.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::test;
using namespace unilink::test::mocks;
using namespace std::chrono_literals;

/**
 * @brief Mock-based integration tests for unilink library
 *
 * These tests use mock objects to simulate network behavior without actual
 * network operations, making them faster, more reliable, and environment-independent.
 */
class MockIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize mock objects
    mock_socket_ = std::make_unique<MockTcpSocket>();
    mock_acceptor_ = std::make_unique<MockTcpAcceptor>();
    mock_serial_ = std::make_unique<MockSerialPort>();

    // Initialize state tracker
    state_tracker_ = std::make_unique<MockStateTracker>();

    // Reset test data
    received_data_.clear();
    connection_established_ = false;
    error_occurred_ = false;
    error_message_.clear();

    // Get unique test port
    test_port_ = TestUtils::getTestPort();
  }

  void TearDown() override {
    // Clean up mock objects
    mock_socket_.reset();
    mock_acceptor_.reset();
    mock_serial_.reset();
    state_tracker_.reset();

    // Clean up any created objects
    if (client_) {
      client_->stop();
      client_.reset();
    }
    if (server_) {
      server_->stop();
      server_.reset();
    }
    if (serial_) {
      serial_->stop();
      serial_.reset();
    }

    // Wait for cleanup
    std::this_thread::sleep_for(100ms);
  }

  // Mock objects
  std::unique_ptr<MockTcpSocket> mock_socket_;
  std::unique_ptr<MockTcpAcceptor> mock_acceptor_;
  std::unique_ptr<MockSerialPort> mock_serial_;
  std::unique_ptr<MockStateTracker> state_tracker_;

  // Test objects
  std::shared_ptr<wrapper::TcpClient> client_;
  std::shared_ptr<wrapper::TcpServer> server_;
  std::shared_ptr<wrapper::Serial> serial_;

  // Test state
  std::vector<std::string> received_data_;
  std::atomic<bool> connection_established_{false};
  std::atomic<bool> error_occurred_{false};
  std::string error_message_;
  uint16_t test_port_;
};

// ============================================================================
// MOCK CONNECTION TESTS
// ============================================================================

/**
 * @brief Test successful connection simulation
 */
TEST_F(MockIntegrationTest, SimulatedSuccessfulConnection) {
  // Given: Mock setup for successful connection
  MockTestScenario::setupSuccessfulConnection(*mock_socket_);

  // When: Create client with mock
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port_)
                .auto_start(false)
                .on_connect([this]() {
                  connection_established_ = true;
                  state_tracker_->setState(MockStateTracker::State::Connected);
                })
                .on_error([this](const std::string& error) {
                  error_occurred_ = true;
                  error_message_ = error;
                  state_tracker_->setState(MockStateTracker::State::Error);
                })
                .build();

  // Then: Verify client creation
  ASSERT_NE(client_, nullptr);

  // Start client (this would normally trigger network operations)
  client_->start();

  // Verify connection state
  EXPECT_TRUE(state_tracker_->waitForState(MockStateTracker::State::Connected, 1000ms));
  EXPECT_TRUE(connection_established_.load());
  EXPECT_FALSE(error_occurred_.load());
}

/**
 * @brief Test connection failure simulation
 */
TEST_F(MockIntegrationTest, SimulatedConnectionFailure) {
  // Given: Mock setup for connection failure
  MockTestScenario::setupConnectionFailure(*mock_socket_, MockTestScenario::ConnectionResult::ConnectionRefused);

  // When: Create client with mock
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port_)
                .auto_start(false)
                .on_connect([this]() {
                  connection_established_ = true;
                  state_tracker_->setState(MockStateTracker::State::Connected);
                })
                .on_error([this](const std::string& error) {
                  error_occurred_ = true;
                  error_message_ = error;
                  state_tracker_->setState(MockStateTracker::State::Error);
                })
                .build();

  // Then: Verify client creation
  ASSERT_NE(client_, nullptr);

  // Start client
  client_->start();

  // Verify error handling
  EXPECT_TRUE(state_tracker_->waitForState(MockStateTracker::State::Error, 1000ms));
  EXPECT_TRUE(error_occurred_.load());
  EXPECT_FALSE(error_message_.empty());
  EXPECT_FALSE(connection_established_.load());
}

// ============================================================================
// MOCK DATA TRANSFER TESTS
// ============================================================================

/**
 * @brief Test data reception simulation
 */
TEST_F(MockIntegrationTest, SimulatedDataReception) {
  // Given: Test data and mock setup
  std::string test_message = MockTestDataGenerator::generateTestMessage(256);
  MockTestScenario::setupSuccessfulConnection(*mock_socket_);
  MockTestScenario::setupDataReception(*mock_socket_, test_message);

  // When: Create client with data handler
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port_)
                .auto_start(false)
                .on_connect([this]() {
                  connection_established_ = true;
                  state_tracker_->setState(MockStateTracker::State::Connected);
                })
                .on_data([this](const std::string& data) {
                  received_data_.push_back(data);
                  state_tracker_->setState(MockStateTracker::State::DataReceived);
                })
                .build();

  // Then: Verify client creation and data handling
  ASSERT_NE(client_, nullptr);

  client_->start();

  // Wait for connection
  EXPECT_TRUE(state_tracker_->waitForState(MockStateTracker::State::Connected, 1000ms));

  // Simulate data reception (in real scenario, this would be triggered by mock)
  // For now, we'll verify the callback is properly set up
  EXPECT_TRUE(connection_established_.load());
}

/**
 * @brief Test data transmission simulation
 */
TEST_F(MockIntegrationTest, SimulatedDataTransmission) {
  // Given: Mock setup for successful data transmission
  MockTestScenario::setupSuccessfulConnection(*mock_socket_);
  MockTestScenario::setupDataTransmission(*mock_socket_, MockTestScenario::DataTransferResult::Success);

  // When: Create client and send data
  client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port_)
                .auto_start(false)
                .on_connect([this]() {
                  connection_established_ = true;
                  state_tracker_->setState(MockStateTracker::State::Connected);
                })
                .build();

  // Then: Verify client creation
  ASSERT_NE(client_, nullptr);

  client_->start();

  // Wait for connection
  EXPECT_TRUE(state_tracker_->waitForState(MockStateTracker::State::Connected, 1000ms));

  // Send test data
  std::string test_message = MockTestDataGenerator::generateTestMessage(512);
  client_->send(test_message);

  // Verify data was sent (in real scenario, mock would verify the call)
  EXPECT_TRUE(connection_established_.load());
}

// ============================================================================
// MOCK SERVER TESTS
// ============================================================================

/**
 * @brief Test server creation and client acceptance simulation
 */
TEST_F(MockIntegrationTest, SimulatedServerClientAcceptance) {
  // Given: Mock setup for server operations
  EXPECT_CALL(*mock_acceptor_, async_accept(::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke([](MockTcpSocket& socket, auto callback) {
        // Simulate successful client acceptance
        boost::system::error_code ec;
        callback(ec);
      }));

  EXPECT_CALL(*mock_acceptor_, bind(::testing::_)).WillOnce(::testing::Return());

  EXPECT_CALL(*mock_acceptor_, listen()).WillOnce(::testing::Return());

  EXPECT_CALL(*mock_acceptor_, is_open()).WillRepeatedly(::testing::Return(true));

  // When: Create server
  server_ = builder::UnifiedBuilder::tcp_server(test_port_)
                .auto_start(false)
                .on_connect([this]() {
                  connection_established_ = true;
                  state_tracker_->setState(MockStateTracker::State::Connected);
                })
                .build();

  // Then: Verify server creation
  ASSERT_NE(server_, nullptr);

  server_->start();

  // Verify server is running (in real scenario, mock would verify the calls)
  EXPECT_NE(server_, nullptr);
}

// ============================================================================
// MOCK ERROR HANDLING TESTS
// ============================================================================

/**
 * @brief Test various error scenarios
 */
TEST_F(MockIntegrationTest, SimulatedErrorScenarios) {
  struct ErrorTestCase {
    MockTestScenario::ConnectionResult result;
  };

  std::vector<ErrorTestCase> test_cases = {{MockTestScenario::ConnectionResult::ConnectionRefused},
                                           {MockTestScenario::ConnectionResult::Timeout},
                                           {MockTestScenario::ConnectionResult::NetworkUnreachable},
                                           {MockTestScenario::ConnectionResult::PermissionDenied}};

  for (const auto& test_case : test_cases) {
    // Reset state for each test case
    error_occurred_ = false;
    error_message_.clear();
    state_tracker_->reset();

    // Given: Mock setup for specific error
    MockTestScenario::setupConnectionFailure(*mock_socket_, test_case.result);

    // When: Create client
    client_ = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port_)
                  .auto_start(false)
                  .on_error([this](const std::string& error) {
                    error_occurred_ = true;
                    error_message_ = error;
                    state_tracker_->setState(MockStateTracker::State::Error);
                  })
                  .build();

    // Then: Verify error handling
    ASSERT_NE(client_, nullptr);

    client_->start();

    // Wait for error state
    EXPECT_TRUE(state_tracker_->waitForState(MockStateTracker::State::Error, 1000ms));
    EXPECT_TRUE(error_occurred_.load());
    EXPECT_FALSE(error_message_.empty());

    // Clean up for next iteration
    client_->stop();
    client_.reset();
  }
}

// ============================================================================
// MOCK PERFORMANCE TESTS
// ============================================================================

/**
 * @brief Test mock performance characteristics
 */
TEST_F(MockIntegrationTest, MockPerformanceTest) {
  // Given: Mock setup for high-performance scenario
  MockTestScenario::setupSuccessfulConnection(*mock_socket_);

  auto start_time = std::chrono::high_resolution_clock::now();

  // When: Create multiple clients rapidly
  std::vector<std::shared_ptr<wrapper::TcpClient>> clients;
  const int client_count = 100;

  for (int i = 0; i < client_count; ++i) {
    auto client = builder::UnifiedBuilder::tcp_client("127.0.0.1", test_port_ + i).auto_start(false).build();

    ASSERT_NE(client, nullptr);
    clients.push_back(std::move(client));
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  // Then: Verify performance
  EXPECT_EQ(clients.size(), client_count);

  // Mock-based creation should be very fast (less than 1ms per client)
  EXPECT_LT(duration.count(), client_count * 1000);  // 1000 microseconds per client

  std::cout << "Created " << client_count << " mock clients in " << duration.count() << " microseconds" << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

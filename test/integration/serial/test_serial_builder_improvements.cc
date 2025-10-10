#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "unilink/builder/unified_builder.hpp"
#include "unilink/common/constants.hpp"
#include "unilink/common/exceptions.hpp"
#include "unilink/common/input_validator.hpp"
#include "unilink/unilink.hpp"

using namespace unilink;
using namespace unilink::common;
using namespace unilink::builder;
using namespace std::chrono_literals;

// Test fixture for common setup
class SerialBuilderImprovementsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset error handler for clean test runs
    ErrorHandler::instance().clear_callbacks();
    ErrorHandler::instance().reset_stats();
    ErrorHandler::instance().set_min_error_level(ErrorLevel::INFO);
  }
};

// ============================================================================
// SERIAL BUILDER EXCEPTION SAFETY TESTS
// ============================================================================

/**
 * @brief Test SerialBuilder exception safety during construction
 */
TEST_F(SerialBuilderImprovementsTest, SerialBuilderExceptionSafety) {
  // Test invalid device path
  EXPECT_THROW(auto serial = unilink::serial("", 115200).build(), BuilderException);

  // Test invalid baud rate
  EXPECT_THROW(auto serial = unilink::serial("/dev/ttyUSB0", 0).build(), BuilderException);
  EXPECT_THROW(auto serial = unilink::serial("/dev/ttyUSB0", constants::MAX_BAUD_RATE + 1).build(),
               BuilderException);

  // Test invalid retry interval
  EXPECT_THROW(auto serial = unilink::serial("/dev/ttyUSB0", 115200).retry_interval(0).build(),
               BuilderException);
  EXPECT_THROW(
      auto serial =
          unilink::serial("/dev/ttyUSB0", 115200).retry_interval(constants::MAX_RETRY_INTERVAL_MS + 1).build(),
      BuilderException);

  // Test valid configuration
  EXPECT_NO_THROW(auto serial =
                      unilink::serial("/dev/ttyUSB0", 115200).retry_interval(1000).auto_start(false).build());
}

/**
 * @brief Test SerialBuilder input validation
 */
TEST_F(SerialBuilderImprovementsTest, SerialBuilderInputValidation) {
  // Test device path validation
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyUSB0"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("COM1"));
  EXPECT_NO_THROW(InputValidator::validate_device_path("/dev/ttyACM0"));

  EXPECT_THROW(InputValidator::validate_device_path(""), ValidationException);
  EXPECT_THROW(InputValidator::validate_device_path(std::string(constants::MAX_DEVICE_PATH_LENGTH + 1, 'a')),
               ValidationException);

  // Test baud rate validation
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(9600));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(115200));
  EXPECT_NO_THROW(InputValidator::validate_baud_rate(constants::MAX_BAUD_RATE));

  EXPECT_THROW(InputValidator::validate_baud_rate(constants::MIN_BAUD_RATE - 1), ValidationException);
  EXPECT_THROW(InputValidator::validate_baud_rate(constants::MAX_BAUD_RATE + 1), ValidationException);

  // Test retry interval validation
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(constants::MIN_RETRY_INTERVAL_MS));
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(constants::DEFAULT_RETRY_INTERVAL_MS));
  EXPECT_NO_THROW(InputValidator::validate_retry_interval(constants::MAX_RETRY_INTERVAL_MS));

  EXPECT_THROW(InputValidator::validate_retry_interval(constants::MIN_RETRY_INTERVAL_MS - 1), ValidationException);
  EXPECT_THROW(InputValidator::validate_retry_interval(constants::MAX_RETRY_INTERVAL_MS + 1), ValidationException);
}

/**
 * @brief Test SerialBuilder method chaining with validation
 */
TEST_F(SerialBuilderImprovementsTest, SerialBuilderMethodChaining) {
  // Test valid method chaining
  EXPECT_NO_THROW({
    auto serial = unilink::serial("/dev/ttyUSB0", 115200)
                      .auto_start(false)
                      .auto_manage(false)
                      .retry_interval(1000)
                      .on_data([](const std::string&) {})
                      .on_connect([]() {})
                      .on_disconnect([]() {})
                      .on_error([](const std::string&) {})
                      .build();
    EXPECT_NE(serial, nullptr);
  });

  // Test invalid retry interval in method chaining
  EXPECT_THROW(
      {
        auto serial = unilink::serial("/dev/ttyUSB0", 115200)
                          .retry_interval(0)  // Invalid
                          .build();
      },
      BuilderException);
}

/**
 * @brief Test SerialBuilder constants usage
 */
TEST_F(SerialBuilderImprovementsTest, SerialBuilderConstantsUsage) {
  // Test that constants are properly defined
  EXPECT_EQ(common::constants::DEFAULT_RETRY_INTERVAL_MS, 2000);
  EXPECT_EQ(common::constants::MIN_BAUD_RATE, 50);
  EXPECT_EQ(common::constants::MAX_BAUD_RATE, 4000000);
  EXPECT_EQ(common::constants::MIN_RETRY_INTERVAL_MS, 100);
  EXPECT_EQ(common::constants::MAX_RETRY_INTERVAL_MS, 300000);  // Fixed: 5 minutes = 300000ms
  EXPECT_EQ(common::constants::MAX_DEVICE_PATH_LENGTH, 256);
}

/**
 * @brief Test SerialBuilder error messages
 */
TEST_F(SerialBuilderImprovementsTest, SerialBuilderErrorMessages) {
  try {
    auto serial = unilink::serial("", 115200).build();
    FAIL() << "Expected BuilderException to be thrown";
  } catch (const BuilderException& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("Invalid Serial parameters"));
    EXPECT_THAT(e.what(), ::testing::HasSubstr("device_path"));
    EXPECT_THAT(e.what(), ::testing::HasSubstr("cannot be empty"));
  }

  try {
    auto serial = unilink::serial("/dev/ttyUSB0", 115200).retry_interval(0).build();
    FAIL() << "Expected BuilderException to be thrown";
  } catch (const BuilderException& e) {
    EXPECT_THAT(e.what(), ::testing::HasSubstr("Invalid retry interval"));
    EXPECT_THAT(e.what(), ::testing::HasSubstr("retry_interval_ms"));
  }
}

/**
 * @brief Test end-to-end SerialBuilder functionality
 */
TEST_F(SerialBuilderImprovementsTest, SerialBuilderEndToEnd) {
  // Test that valid configurations work
  EXPECT_NO_THROW({
    auto serial = unilink::serial("/dev/ttyUSB0", 115200).auto_start(false).retry_interval(1000).build();
    EXPECT_NE(serial, nullptr);
  });

  // Test that invalid configurations throw appropriate exceptions
  EXPECT_THROW(auto serial = unilink::serial("", 115200).build(), BuilderException);
  EXPECT_THROW(auto serial = unilink::serial("/dev/ttyUSB0", 0).build(), BuilderException);
}

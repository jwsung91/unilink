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

#include <chrono>
#include <iostream>
#include <thread>

#include "unilink/unilink.hpp"

// Example namespace usage - using namespace for simplicity in examples
using namespace unilink;

int main() {
  std::cout << "=== Unilink Error Handling System Usage Example ===" << std::endl;

  // 1. Error handler setup
  std::cout << "\n1. Error handler setup" << std::endl;

  // Set minimum error level
  common::ErrorHandler::instance().set_min_error_level(common::ErrorLevel::INFO);

  // Register error callback
  common::ErrorHandler::instance().register_callback([](const common::ErrorInfo& error) {
    std::cout << "🚨 Error occurred: " << error.get_summary() << std::endl;

    if (error.level == common::ErrorLevel::CRITICAL) {
      std::cout << "   ⚠️  Critical error - immediate action required!" << std::endl;
    }
  });

  // 2. Various error type tests
  std::cout << "\n2. Various error type tests" << std::endl;

  // Connection error
  boost::system::error_code ec(boost::system::errc::connection_refused, boost::system::system_category());
  common::error_reporting::report_connection_error("tcp_client", "connect", ec, true);

  // Communication error
  common::error_reporting::report_communication_error("tcp_server", "read", "Data read failure", false);

  // Configuration error
  common::error_reporting::report_configuration_error("serial", "validate", "Invalid baud rate setting");

  // Memory error
  common::error_reporting::report_memory_error("buffer", "allocate", "Memory allocation failed");

  // System error
  common::error_reporting::report_system_error("io_context", "run", "IO context execution failed", ec);

  // Warning
  common::error_reporting::report_warning("config", "load", "Configuration file not found, using defaults");

  // Info
  common::error_reporting::report_info("startup", "init", "Application initialization completed");

  // 3. Error statistics check
  std::cout << "\n3. Error statistics check" << std::endl;

  auto stats = common::ErrorHandler::instance().get_error_stats();
  std::cout << "Total errors: " << stats.total_errors << std::endl;
  std::cout << "INFO: " << stats.errors_by_level[0] << std::endl;
  std::cout << "WARNING: " << stats.errors_by_level[1] << std::endl;
  std::cout << "ERROR: " << stats.errors_by_level[2] << std::endl;
  std::cout << "CRITICAL: " << stats.errors_by_level[3] << std::endl;

  std::cout << "Connection errors: " << stats.errors_by_category[0] << std::endl;
  std::cout << "Communication errors: " << stats.errors_by_category[1] << std::endl;
  std::cout << "Configuration errors: " << stats.errors_by_category[2] << std::endl;
  std::cout << "Memory errors: " << stats.errors_by_category[3] << std::endl;
  std::cout << "System errors: " << stats.errors_by_category[4] << std::endl;

  // 4. Component-specific error queries
  std::cout << "\n4. Component-specific error queries" << std::endl;

  auto tcp_errors = common::ErrorHandler::instance().get_errors_by_component("tcp_client");
  std::cout << "TCP client error count: " << tcp_errors.size() << std::endl;

  auto serial_errors = common::ErrorHandler::instance().get_errors_by_component("serial");
  std::cout << "Serial error count: " << serial_errors.size() << std::endl;

  // 5. Recent error queries
  std::cout << "\n5. Recent error queries" << std::endl;

  auto recent_errors = common::ErrorHandler::instance().get_recent_errors(3);
  std::cout << "Recent 3 errors:" << std::endl;
  for (const auto& error : recent_errors) {
    std::cout << "  - " << error.get_summary() << std::endl;
  }

  // 6. Real TCP server/client error testing
  std::cout << "\n6. Real TCP server/client error testing" << std::endl;

  // Try connecting to invalid port (will cause error)
  auto bad_client =
      tcp_client("127.0.0.1", 1)  // Invalid port
          .auto_start(true)
          .on_error([](const std::string& error) { std::cout << "Client error callback: " << error << std::endl; })
          .build();

  if (bad_client) {
    UNILINK_LOG_INFO("example", "test", "Attempting connection to invalid port...");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    bad_client->stop();
  }

  // 7. Error handler disable/enable
  std::cout << "\n7. Error handler disable/enable" << std::endl;

  common::ErrorHandler::instance().set_enabled(false);
  common::error_reporting::report_info("test", "disabled", "This message is not visible");

  common::ErrorHandler::instance().set_enabled(true);
  common::error_reporting::report_info("test", "enabled", "Error handler re-enabled");

  // 8. Error statistics reset
  std::cout << "\n8. Error statistics reset" << std::endl;

  common::ErrorHandler::instance().reset_stats();
  auto reset_stats = common::ErrorHandler::instance().get_error_stats();
  std::cout << "Total errors after reset: " << reset_stats.total_errors << std::endl;

  std::cout << "\n=== Error handling example completed ===" << std::endl;

  return 0;
}

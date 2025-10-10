#pragma once

#include <stdexcept>
#include <string>

namespace unilink {
namespace common {

/**
 * @brief Base exception class for all unilink exceptions
 *
 * Provides a common base for all exceptions thrown by the unilink library.
 * Includes additional context information for better error reporting.
 */
class UnilinkException : public std::runtime_error {
 public:
  explicit UnilinkException(const std::string& message, const std::string& component = "",
                            const std::string& operation = "")
      : std::runtime_error(message), component_(component), operation_(operation) {}

  const std::string& get_component() const noexcept { return component_; }
  const std::string& get_operation() const noexcept { return operation_; }

  std::string get_full_message() const {
    std::string full_msg = what();
    if (!component_.empty()) {
      full_msg = "[" + component_ + "] " + full_msg;
    }
    if (!operation_.empty()) {
      full_msg += " (operation: " + operation_ + ")";
    }
    return full_msg;
  }

 private:
  std::string component_;
  std::string operation_;
};

/**
 * @brief Exception thrown during builder operations
 *
 * Indicates errors that occur during the construction or configuration
 * of communication channels using the Builder pattern.
 */
class BuilderException : public UnilinkException {
 public:
  explicit BuilderException(const std::string& message, const std::string& builder_type = "",
                            const std::string& operation = "")
      : UnilinkException(message, "builder", operation), builder_type_(builder_type) {}

  const std::string& get_builder_type() const noexcept { return builder_type_; }

  std::string get_full_message() const {
    std::string full_msg = UnilinkException::get_full_message();
    if (!builder_type_.empty()) {
      full_msg = "[" + builder_type_ + "] " + full_msg;
    }
    return full_msg;
  }

 private:
  std::string builder_type_;
};

/**
 * @brief Exception thrown during input validation
 *
 * Indicates that input parameters failed validation checks.
 * Provides detailed information about what validation failed.
 */
class ValidationException : public UnilinkException {
 public:
  explicit ValidationException(const std::string& message, const std::string& parameter = "",
                               const std::string& expected = "")
      : UnilinkException(message, "validation", "validate"), parameter_(parameter), expected_(expected) {}

  const std::string& get_parameter() const noexcept { return parameter_; }
  const std::string& get_expected() const noexcept { return expected_; }

  std::string get_full_message() const {
    std::string full_msg = UnilinkException::get_full_message();
    if (!parameter_.empty()) {
      full_msg += " (parameter: " + parameter_ + ")";
    }
    if (!expected_.empty()) {
      full_msg += " (expected: " + expected_ + ")";
    }
    return full_msg;
  }

 private:
  std::string parameter_;
  std::string expected_;
};

/**
 * @brief Exception thrown during memory operations
 *
 * Indicates errors related to memory allocation, deallocation,
 * or memory safety violations.
 */
class MemoryException : public UnilinkException {
 public:
  explicit MemoryException(const std::string& message, size_t size = 0, const std::string& operation = "")
      : UnilinkException(message, "memory", operation), size_(size) {}

  size_t get_size() const noexcept { return size_; }

  std::string get_full_message() const {
    std::string full_msg = UnilinkException::get_full_message();
    if (size_ > 0) {
      full_msg += " (size: " + std::to_string(size_) + " bytes)";
    }
    return full_msg;
  }

 private:
  size_t size_;
};

/**
 * @brief Exception thrown during connection operations
 *
 * Indicates errors that occur during network or serial connection
 * establishment, maintenance, or teardown.
 */
class ConnectionException : public UnilinkException {
 public:
  explicit ConnectionException(const std::string& message, const std::string& connection_type = "",
                               const std::string& operation = "")
      : UnilinkException(message, "connection", operation), connection_type_(connection_type) {}

  const std::string& get_connection_type() const noexcept { return connection_type_; }

  std::string get_full_message() const {
    std::string full_msg = UnilinkException::get_full_message();
    if (!connection_type_.empty()) {
      full_msg = "[" + connection_type_ + "] " + full_msg;
    }
    return full_msg;
  }

 private:
  std::string connection_type_;
};

/**
 * @brief Exception thrown during configuration operations
 *
 * Indicates errors that occur during configuration loading,
 * validation, or application.
 */
class ConfigurationException : public UnilinkException {
 public:
  explicit ConfigurationException(const std::string& message, const std::string& config_section = "",
                                  const std::string& operation = "")
      : UnilinkException(message, "configuration", operation), config_section_(config_section) {}

  const std::string& get_config_section() const noexcept { return config_section_; }

  std::string get_full_message() const {
    std::string full_msg = UnilinkException::get_full_message();
    if (!config_section_.empty()) {
      full_msg += " (section: " + config_section_ + ")";
    }
    return full_msg;
  }

 private:
  std::string config_section_;
};

}  // namespace common
}  // namespace unilink

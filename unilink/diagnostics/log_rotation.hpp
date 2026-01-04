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

#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include "unilink/base/visibility.hpp"

namespace unilink {
namespace diagnostics {

/**
 * @brief Log rotation configuration
 */
struct LogRotationConfig {
  size_t max_file_size_bytes = 10 * 1024 * 1024;    // 10MB default
  size_t max_files = 10;                            // Keep 10 files max
  bool enable_compression = false;                  // Future feature
  std::string file_pattern = "{name}.{index}.log";  // File naming pattern

  LogRotationConfig() = default;

  LogRotationConfig(size_t max_size, size_t max_count) : max_file_size_bytes(max_size), max_files(max_count) {}
};

/**
 * @brief Log rotation manager
 *
 * Handles log file rotation based on size and automatic cleanup
 * of old log files based on count limits.
 */
class UNILINK_API LogRotation {
 public:
  /**
   * @brief Constructor
   * @param config Rotation configuration
   */
  explicit LogRotation(const LogRotationConfig& config = LogRotationConfig{});

  /**
   * @brief Check if current log file needs rotation
   * @param filepath Current log file path
   * @return true if rotation is needed
   */
  bool should_rotate(const std::string& filepath) const;

  /**
   * @brief Perform log rotation
   * @param filepath Current log file path
   * @return New log file path after rotation
   */
  std::string rotate(const std::string& filepath);

  /**
   * @brief Clean up old log files
   * @param base_filepath Base log file path (without index)
   */
  void cleanup_old_files(const std::string& base_filepath);

  /**
   * @brief Get next available log file path
   * @param base_filepath Base log file path
   * @return Next available file path
   */
  std::string get_next_file_path(const std::string& base_filepath) const;

  /**
   * @brief Update rotation configuration
   * @param config New configuration
   */
  void update_config(const LogRotationConfig& config);

  /**
   * @brief Get current configuration
   */
  const LogRotationConfig& get_config() const { return config_; }

  /**
   * @brief Get file size in bytes
   * @param filepath File path to check
   * @return File size in bytes, 0 if file doesn't exist
   */
  static size_t get_file_size(const std::string& filepath);

  /**
   * @brief Get all log files matching pattern
   * @param base_filepath Base log file path
   * @return Vector of matching log file paths, sorted by modification time
   */
  static std::vector<std::string> get_log_files(const std::string& base_filepath);

 private:
  LogRotationConfig config_;
  mutable std::mutex mutex_;

  /**
   * @brief Extract base filename without extension
   * @param filepath Full file path
   * @return Base filename without extension
   */
  std::string get_base_filename(const std::string& filepath) const;

  /**
   * @brief Extract directory path
   * @param filepath Full file path
   * @return Directory path
   */
  std::string get_directory(const std::string& filepath) const;

  /**
   * @brief Get file index from filename
   * @param filename Filename to parse
   * @return File index, -1 if not found
   */
  int get_file_index(const std::string& filename) const;

  /**
   * @brief Generate filename with index
   * @param base_name Base filename
   * @param index File index
   * @return Filename with index
   */
  std::string generate_filename(const std::string& base_name, int index) const;

  /**
   * @brief Sort files by modification time (newest first)
   * @param files Vector of file paths to sort
   */
  void sort_files_by_time(std::vector<std::string>& files) const;
};

}  // namespace diagnostics
}  // namespace unilink

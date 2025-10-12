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

#include <string>

namespace unilink {
namespace common {

/**
 * @brief Platform detection and feature availability
 *
 * This header provides platform detection macros and feature availability
 * functions for different Ubuntu versions and compilers.
 */

// Platform detection macros (set by CMake)
#ifdef UNILINK_UBUNTU_20_04
#define UNILINK_UBUNTU_VERSION 20
#define UNILINK_FEATURE_LEVEL 1  // Basic features only
#elif defined(UNILINK_UBUNTU_22_04)
#define UNILINK_UBUNTU_VERSION 22
#define UNILINK_FEATURE_LEVEL 2  // Standard features
#elif defined(UNILINK_UBUNTU_24_04)
#define UNILINK_UBUNTU_VERSION 24
#define UNILINK_FEATURE_LEVEL 3  // All features
#else
#define UNILINK_UBUNTU_VERSION 0
#define UNILINK_FEATURE_LEVEL 2  // Default to standard
#endif

// Feature availability macros
#define UNILINK_ENABLE_ADVANCED_LOGGING (UNILINK_FEATURE_LEVEL >= 2)
#define UNILINK_ENABLE_PERFORMANCE_MONITORING (UNILINK_FEATURE_LEVEL >= 2)
#define UNILINK_ENABLE_LATEST_OPTIMIZATIONS (UNILINK_FEATURE_LEVEL >= 3)
#define UNILINK_ENABLE_EXPERIMENTAL_FEATURES (UNILINK_FEATURE_LEVEL >= 3)

/**
 * @brief Platform information utilities
 */
class PlatformInfo {
 public:
  /**
   * @brief Get the detected Ubuntu version
   * @return Ubuntu version number (20, 22, 24, or 0 for unknown)
   */
  static int get_ubuntu_version() { return UNILINK_UBUNTU_VERSION; }

  /**
   * @brief Get the feature level
   * @return Feature level (1=basic, 2=standard, 3=all)
   */
  static int get_feature_level() { return UNILINK_FEATURE_LEVEL; }

  /**
   * @brief Get a human-readable platform description
   * @return Platform description string
   */
  static std::string get_platform_description() {
#ifdef UNILINK_UBUNTU_20_04
    return "Ubuntu 20.04 (Limited Features)";
#elif defined(UNILINK_UBUNTU_22_04)
    return "Ubuntu 22.04 (Full Features)";
#elif defined(UNILINK_UBUNTU_24_04)
    return "Ubuntu 24.04 (All Features)";
#else
    return "Unknown Ubuntu Version";
#endif
  }

  /**
   * @brief Check if advanced logging is available
   * @return true if advanced logging is available
   */
  static bool is_advanced_logging_available() { return UNILINK_ENABLE_ADVANCED_LOGGING; }

  /**
   * @brief Check if performance monitoring is available
   * @return true if performance monitoring is available
   */
  static bool is_performance_monitoring_available() { return UNILINK_ENABLE_PERFORMANCE_MONITORING; }

  /**
   * @brief Check if latest optimizations are available
   * @return true if latest optimizations are available
   */
  static bool is_latest_optimizations_available() { return UNILINK_ENABLE_LATEST_OPTIMIZATIONS; }

  /**
   * @brief Check if experimental features are available
   * @return true if experimental features are available
   */
  static bool is_experimental_features_available() { return UNILINK_ENABLE_EXPERIMENTAL_FEATURES; }

  /**
   * @brief Get a warning message for limited support platforms
   * @return Warning message or empty string
   */
  static std::string get_support_warning() {
#ifdef UNILINK_UBUNTU_20_04
    return "WARNING: Running on Ubuntu 20.04 with limited support. "
           "Consider upgrading to Ubuntu 22.04+ for full features.";
#else
    return "";
#endif
  }
};

}  // namespace common
}  // namespace unilink

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

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef wait_callback
#undef wait_callback
#endif
#endif

namespace unilink {
namespace common {

/**
 * @brief Platform detection and feature availability
 *
 * This header provides platform detection macros and feature availability
 * functions for different Ubuntu versions and compilers.
 */

// Platform detection macros (set by CMake or inferred from compiler)
#if defined(UNILINK_UBUNTU_20_04)
#define UNILINK_UBUNTU_VERSION 20
#define UNILINK_FEATURE_LEVEL 1  // Basic features only
#elif defined(UNILINK_UBUNTU_22_04)
#define UNILINK_UBUNTU_VERSION 22
#define UNILINK_FEATURE_LEVEL 2  // Standard features
#elif defined(UNILINK_UBUNTU_24_04)
#define UNILINK_UBUNTU_VERSION 24
#define UNILINK_FEATURE_LEVEL 3  // All features
#elif defined(UNILINK_PLATFORM_WINDOWS) || defined(_WIN32)
#define UNILINK_PLATFORM_WINDOWS 1
#define UNILINK_UBUNTU_VERSION 0
#define UNILINK_FEATURE_LEVEL 2  // Windows matches standard feature set
#elif defined(UNILINK_PLATFORM_MACOS) || defined(__APPLE__)
#define UNILINK_PLATFORM_MACOS 1
#define UNILINK_UBUNTU_VERSION 0
#define UNILINK_FEATURE_LEVEL 2  // macOS matches standard feature set
#elif defined(UNILINK_PLATFORM_POSIX)
#define UNILINK_UBUNTU_VERSION 0
#define UNILINK_FEATURE_LEVEL 2
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
#if defined(UNILINK_UBUNTU_20_04)
    return "Ubuntu 20.04 (Limited Features)";
#elif defined(UNILINK_UBUNTU_22_04)
    return "Ubuntu 22.04 (Full Features)";
#elif defined(UNILINK_UBUNTU_24_04)
    return "Ubuntu 24.04 (All Features)";
#elif defined(UNILINK_PLATFORM_WINDOWS)
    return "Windows (Full Features)";
#elif defined(UNILINK_PLATFORM_MACOS)
    return "macOS (Full Features)";
#elif defined(UNILINK_PLATFORM_POSIX)
    return "POSIX Platform (Standard Features)";
#else
    return "Unknown Platform";
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
#if defined(UNILINK_UBUNTU_20_04)
    return "WARNING: Running on Ubuntu 20.04 with limited support. "
           "Consider upgrading to Ubuntu 22.04+ for full features.";
#elif defined(UNILINK_PLATFORM_WINDOWS)
    return "";
#elif defined(UNILINK_PLATFORM_MACOS)
    return "";
#else
    return "";
#endif
  }
};

}  // namespace common
}  // namespace unilink

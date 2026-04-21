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

#include <chrono>
#include <cstdint>
#include <string>

#include "unilink/base/visibility.hpp"
#include "unilink/builder/ibuilder.hpp"
#include "unilink/wrapper/uds_client/uds_client.hpp"
#include "unilink/wrapper/uds_server/uds_server.hpp"

namespace unilink {
namespace builder {

/**
 * @brief Modernized Builder for UdsClient
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API UdsClientBuilder : public BuilderInterface<wrapper::UdsClient, UdsClientBuilder> {
 public:
  explicit UdsClientBuilder(const std::string& socket_path);

  std::unique_ptr<wrapper::UdsClient> build() override;
  UdsClientBuilder& auto_start(bool auto_start = true) override;

  UdsClientBuilder& retry_interval(std::chrono::milliseconds milliseconds);
  UdsClientBuilder& max_retries(int max_retries);
  UdsClientBuilder& connection_timeout(std::chrono::milliseconds milliseconds);
  UdsClientBuilder& independent_context(bool use_independent = true);

 private:
  std::string socket_path_;
  bool auto_start_;
  bool independent_context_;
  std::chrono::milliseconds retry_interval_;
  int max_retries_;
  std::chrono::milliseconds connection_timeout_;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

/**
 * @brief Modernized Builder for UdsServer
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
class UNILINK_API UdsServerBuilder : public BuilderInterface<wrapper::UdsServer, UdsServerBuilder> {
 public:
  explicit UdsServerBuilder(const std::string& socket_path);

  std::unique_ptr<wrapper::UdsServer> build() override;
  UdsServerBuilder& auto_start(bool auto_start = true) override;

  /**
   * @brief Helper for client disconnection events
   */
  UdsServerBuilder& on_client_disconnect(std::function<void(const wrapper::ConnectionContext&)> handler) {
    return on_disconnect(std::move(handler));
  }

  UdsServerBuilder& independent_context(bool use_independent = true);
  UdsServerBuilder& idle_timeout(std::chrono::milliseconds timeout);
  UdsServerBuilder& max_clients(size_t max);
  UdsServerBuilder& unlimited_clients();

 private:
  std::string socket_path_;
  bool auto_start_;
  bool independent_context_;
  std::chrono::milliseconds idle_timeout_{0};
  size_t max_clients_;
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace builder
}  // namespace unilink

#include "unilink/builder/serial_builder.hpp"

#include "unilink/common/io_context_manager.hpp"

namespace unilink {
namespace builder {

SerialBuilder::SerialBuilder(const std::string& device, uint32_t baud_rate)
    : device_(device),
      baud_rate_(baud_rate),
      auto_start_(false),
      auto_manage_(false),
      use_independent_context_(false),
      retry_interval_ms_(2000) {}

std::unique_ptr<wrapper::Serial> SerialBuilder::build() {
  // IoContext management
  if (use_independent_context_) {
    // Use independent IoContext (for test isolation)
    // Create independent context through IoContextManager
    auto independent_context = common::IoContextManager::instance().create_independent_context();
    // Currently maintaining default implementation, can be extended in future for wrapper to accept independent context
  } else {
    // Default behavior (Serial manages IoContext independently)
  }

  auto serial = std::make_unique<wrapper::Serial>(device_, baud_rate_);

  // Apply configuration
  if (auto_start_) {
    serial->auto_start(true);
  }

  if (auto_manage_) {
    serial->auto_manage(true);
  }

  // Set callbacks
  if (on_data_) {
    serial->on_data(on_data_);
  }

  if (on_connect_) {
    serial->on_connect(on_connect_);
  }

  if (on_disconnect_) {
    serial->on_disconnect(on_disconnect_);
  }

  if (on_error_) {
    serial->on_error(on_error_);
  }

  // Set retry interval
  serial->set_retry_interval(std::chrono::milliseconds(retry_interval_ms_));

  return serial;
}

SerialBuilder& SerialBuilder::auto_start(bool auto_start) {
  auto_start_ = auto_start;
  return *this;
}

SerialBuilder& SerialBuilder::auto_manage(bool auto_manage) {
  auto_manage_ = auto_manage;
  return *this;
}

SerialBuilder& SerialBuilder::on_data(std::function<void(const std::string&)> handler) {
  on_data_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_connect(std::function<void()> handler) {
  on_connect_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_disconnect(std::function<void()> handler) {
  on_disconnect_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::on_error(std::function<void(const std::string&)> handler) {
  on_error_ = std::move(handler);
  return *this;
}

SerialBuilder& SerialBuilder::use_independent_context(bool use_independent) {
  use_independent_context_ = use_independent;
  return *this;
}

SerialBuilder& SerialBuilder::retry_interval(unsigned interval_ms) {
  retry_interval_ms_ = interval_ms;
  return *this;
}

}  // namespace builder
}  // namespace unilink

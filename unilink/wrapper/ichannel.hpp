#pragma once

#include <functional>
#include <memory>
#include <string>

namespace unilink {
namespace wrapper {

// Common interface for all Wrapper communication classes
class ChannelInterface {
 public:
  using DataHandler = std::function<void(const std::string&)>;
  using ConnectHandler = std::function<void()>;
  using DisconnectHandler = std::function<void()>;
  using ErrorHandler = std::function<void(const std::string&)>;

  virtual ~ChannelInterface() = default;

  // Common methods
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void send(const std::string& data) = 0;
  virtual void send_line(const std::string& line) = 0;
  virtual bool is_connected() const = 0;

  // Event handler setup
  virtual ChannelInterface& on_data(DataHandler handler) = 0;
  virtual ChannelInterface& on_connect(ConnectHandler handler) = 0;
  virtual ChannelInterface& on_disconnect(DisconnectHandler handler) = 0;
  virtual ChannelInterface& on_error(ErrorHandler handler) = 0;

  // Convenience methods
  virtual ChannelInterface& auto_start(bool start = true) = 0;
  virtual ChannelInterface& auto_manage(bool manage = true) = 0;
};

}  // namespace wrapper
}  // namespace unilink

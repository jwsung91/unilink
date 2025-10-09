#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "unilink/interface/channel.hpp"
#include "unilink/wrapper/ichannel.hpp"

namespace unilink {
namespace wrapper {

class TcpClient : public ChannelInterface {
 public:
  TcpClient(const std::string& host, uint16_t port);
  explicit TcpClient(std::shared_ptr<interface::Channel> channel);
  ~TcpClient() override;

  // IChannel implementation
  void start() override;
  void stop() override;
  void send(const std::string& data) override;
  void send_line(const std::string& line) override;
  bool is_connected() const override;

  ChannelInterface& on_data(DataHandler handler) override;
  ChannelInterface& on_connect(ConnectHandler handler) override;
  ChannelInterface& on_disconnect(DisconnectHandler handler) override;
  ChannelInterface& on_error(ErrorHandler handler) override;

  ChannelInterface& auto_start(bool start = true) override;
  ChannelInterface& auto_manage(bool manage = true) override;

  // TCP client specific methods
  void set_retry_interval(std::chrono::milliseconds interval);
  void set_max_retries(int max_retries);
  void set_connection_timeout(std::chrono::milliseconds timeout);

 private:
  void setup_internal_handlers();
  void notify_state_change(common::LinkState state);

 private:
  std::string host_;
  uint16_t port_;
  std::shared_ptr<interface::Channel> channel_;

  // Event handlers
  DataHandler data_handler_;
  ConnectHandler connect_handler_;
  DisconnectHandler disconnect_handler_;
  ErrorHandler error_handler_;

  // Configuration
  bool auto_start_ = false;
  bool auto_manage_ = false;
  bool started_ = false;

  // TCP client specific configuration
  std::chrono::milliseconds retry_interval_{2000};
  int max_retries_ = -1;  // -1 means unlimited
  std::chrono::milliseconds connection_timeout_{5000};
};

}  // namespace wrapper
}  // namespace unilink

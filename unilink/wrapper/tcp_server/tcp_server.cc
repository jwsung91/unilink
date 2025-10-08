#include "unilink/wrapper/tcp_server/tcp_server.hpp"
#include "unilink/config/tcp_server_config.hpp"
#include "unilink/factory/channel_factory.hpp"
#include "unilink/transport/tcp_server/tcp_server.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace unilink {
namespace wrapper {

TcpServer::TcpServer(uint16_t port) 
    : port_(port), channel_(nullptr) {
    // Channel은 나중에 start() 시점에 생성
}

TcpServer::TcpServer(std::shared_ptr<interface::Channel> channel)
    : port_(0), channel_(channel) {
    setup_internal_handlers();
}

void TcpServer::start() {
    if (started_) return;
    
    if (!channel_) {
        // 개선된 Factory 사용
        config::TcpServerConfig config;
        config.port = port_;
        channel_ = factory::ChannelFactory::create(config);
        setup_internal_handlers();
    }
    
    channel_->start();
    started_ = true;
}

void TcpServer::stop() {
    if (!started_ || !channel_) return;
    
    channel_->stop();
    // Allow async operations to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    channel_.reset();
    started_ = false;
}

void TcpServer::send(const std::string& data) {
    if (is_connected() && channel_) {
        auto binary_data = common::safe_convert::string_to_uint8(data);
        channel_->async_write_copy(binary_data.data(), binary_data.size());
    }
}

bool TcpServer::is_connected() const {
    return channel_ && channel_->is_connected();
}

ChannelInterface& TcpServer::on_data(DataHandler handler) {
    on_data_ = std::move(handler);
    return *this;
}

ChannelInterface& TcpServer::on_connect(ConnectHandler handler) {
    on_connect_ = std::move(handler);
    return *this;
}

ChannelInterface& TcpServer::on_disconnect(DisconnectHandler handler) {
    on_disconnect_ = std::move(handler);
    return *this;
}

ChannelInterface& TcpServer::on_error(ErrorHandler handler) {
    on_error_ = std::move(handler);
    return *this;
}

ChannelInterface& TcpServer::auto_start(bool start) {
    auto_start_ = start;
    if (start && !started_) {
        this->start();
    }
    return *this;
}

ChannelInterface& TcpServer::auto_manage(bool manage) {
    auto_manage_ = manage;
    return *this;
}

void TcpServer::send_line(const std::string& line) {
    send(line + "\n");
}

// void ImprovedTcpServer::send_binary(const std::vector<uint8_t>& data) {
//     if (is_connected() && channel_) {
//         channel_->async_write_copy(data.data(), data.size());
//     }
// }

void TcpServer::setup_internal_handlers() {
    if (!channel_) return;
    
    channel_->on_bytes([this](const uint8_t* data, size_t size) {
        handle_bytes(data, size);
    });
    
    channel_->on_state([this](common::LinkState state) {
        handle_state(state);
    });
}

void TcpServer::handle_bytes(const uint8_t* data, size_t size) {
    if (on_data_) {
        std::string str_data = common::safe_convert::uint8_to_string(data, size);
        on_data_(str_data);
    }
}

void TcpServer::handle_state(common::LinkState state) {
    switch (state) {
        case common::LinkState::Connected:
            if (on_connect_) {
                on_connect_();
            }
            break;
        case common::LinkState::Closed:
            if (on_disconnect_) {
                on_disconnect_();
            }
            break;
        case common::LinkState::Error:
            if (on_error_) {
                on_error_("Connection error");
            }
            break;
        default:
            break;
    }
}

// 멀티 클라이언트 지원 메서드 구현
void TcpServer::broadcast(const std::string& message) {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->broadcast(message);
    }
  }
}

void TcpServer::send_to_client(size_t client_id, const std::string& message) {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->send_to_client(client_id, message);
    }
  }
}

size_t TcpServer::get_client_count() const {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      return transport_server->get_client_count();
    }
  }
  return 0;
}

std::vector<size_t> TcpServer::get_connected_clients() const {
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      return transport_server->get_connected_clients();
    }
  }
  return {};
}

TcpServer& TcpServer::on_multi_connect(MultiClientConnectHandler handler) {
  on_multi_connect_ = std::move(handler);
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_connect([this](size_t client_id, const std::string& client_info) {
        if (on_multi_connect_) {
          on_multi_connect_(client_id, client_info);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::on_multi_data(MultiClientDataHandler handler) {
  on_multi_data_ = std::move(handler);
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_data([this](size_t client_id, const std::string& data) {
        if (on_multi_data_) {
          on_multi_data_(client_id, data);
        }
      });
    }
  }
  return *this;
}

TcpServer& TcpServer::on_multi_disconnect(MultiClientDisconnectHandler handler) {
  on_multi_disconnect_ = std::move(handler);
  if (channel_) {
    auto transport_server = std::dynamic_pointer_cast<transport::TcpServer>(channel_);
    if (transport_server) {
      transport_server->on_multi_disconnect([this](size_t client_id) {
        if (on_multi_disconnect_) {
          on_multi_disconnect_(client_id);
        }
      });
    }
  }
  return *this;
}

} // namespace wrapper
} // namespace unilink

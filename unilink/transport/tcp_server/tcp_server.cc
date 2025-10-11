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

#include "unilink/transport/tcp_server/tcp_server.hpp"

#include <future>
#include <iostream>

#include "unilink/common/io_context_manager.hpp"
#include "unilink/transport/tcp_server/boost_tcp_acceptor.hpp"

namespace unilink {
namespace transport {

namespace net = boost::asio;
using tcp = net::ip::tcp;

// Specific namespace aliases instead of using namespace
using common::IoContextManager;
using common::LinkState;
using common::ThreadSafeLinkState;
using config::TcpServerConfig;
using interface::Channel;
using interface::TcpAcceptorInterface;
using namespace common;  // For error_reporting namespace

TcpServer::TcpServer(const TcpServerConfig& cfg)
    : owned_ioc_(nullptr),
      owns_ioc_(false),
      ioc_(common::IoContextManager::instance().get_context()),
      acceptor_(nullptr),
      cfg_(cfg),
      max_clients_(0),
      client_limit_enabled_(false),
      current_session_(nullptr) {
  // Create acceptor after all members are initialized
  try {
    acceptor_ = std::make_unique<BoostTcpAcceptor>(ioc_);
  } catch (const std::exception& e) {
    throw std::runtime_error("Failed to create TCP acceptor: " + std::string(e.what()));
  }
}

TcpServer::TcpServer(const TcpServerConfig& cfg, std::unique_ptr<interface::TcpAcceptorInterface> acceptor,
                     net::io_context& ioc)
    : owned_ioc_(nullptr),
      owns_ioc_(false),
      ioc_(ioc),
      acceptor_(std::move(acceptor)),
      cfg_(cfg),
      max_clients_(0),
      client_limit_enabled_(false),
      current_session_(nullptr) {
  // Ensure acceptor is properly initialized
  if (!acceptor_) {
    throw std::runtime_error("Failed to create TCP acceptor");
  }
}

TcpServer::~TcpServer() {
  // Ensure proper cleanup even if stop() wasn't called explicitly
  if (!state_.is_state(LinkState::Closed)) {
    stop();
  }

  // No need to clean up io_context as it's shared and managed by IoContextManager
}

void TcpServer::start() {
  if (!acceptor_) {
    UNILINK_LOG_ERROR("tcp_server", "start", "Acceptor is null");
    error_reporting::report_system_error("tcp_server", "start", "Acceptor is null");
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  if (owns_ioc_) {
    ioc_thread_ = std::thread([this] { ioc_.run(); });
  }
  auto self = shared_from_this();
  net::post(ioc_, [self] { self->attempt_port_binding(0); });
}

void TcpServer::stop() {
  if (owns_ioc_ && ioc_thread_.joinable()) {
    auto self = shared_from_this();
    std::promise<void> cleanup_promise;
    auto cleanup_future = cleanup_promise.get_future();

    net::post(ioc_, [self, &cleanup_promise] {
      boost::system::error_code ec;
      if (self->acceptor_ && self->acceptor_->is_open()) {
        self->acceptor_->close(ec);
      }
      // 모든 세션 정리
      {
        std::lock_guard<std::mutex> lock(self->sessions_mutex_);
        self->sessions_.clear();
      }
      if (self->current_session_) self->current_session_.reset();
      cleanup_promise.set_value();
    });

    // 정리 작업 완료 대기
    cleanup_future.wait();

    ioc_.stop();
    ioc_thread_.join();
    // Reset io_context to clear any remaining work
    ioc_.restart();
  } else {
    // If server was never started, just clean up
    boost::system::error_code ec;
    if (acceptor_ && acceptor_->is_open()) {
      acceptor_->close(ec);
    }
    // 모든 세션 정리
    {
      std::lock_guard<std::mutex> lock(sessions_mutex_);
      sessions_.clear();
    }
    if (current_session_) current_session_.reset();
  }
  state_.set_state(LinkState::Closed);
  // Don't call notify_state() in stop() as it may cause issues with callbacks
  // during destruction
}

bool TcpServer::is_connected() const { return current_session_ && current_session_->alive(); }

void TcpServer::async_write_copy(const uint8_t* data, size_t size) {
  if (current_session_ && current_session_->alive()) {
    current_session_->async_write_copy(data, size);
  }
  // If no session or session is not alive, the write is silently dropped
}

void TcpServer::on_bytes(OnBytes cb) {
  on_bytes_ = std::move(cb);
  if (current_session_) current_session_->on_bytes(on_bytes_);
}
void TcpServer::on_state(OnState cb) { on_state_ = std::move(cb); }
void TcpServer::on_backpressure(OnBackpressure cb) {
  on_bp_ = std::move(cb);
  if (current_session_) current_session_->on_backpressure(on_bp_);
}

void TcpServer::attempt_port_binding(int retry_count) {
  boost::system::error_code ec;

  // Log retry configuration for debugging
  if (retry_count > 0) {
    UNILINK_LOG_DEBUG("tcp_server", "bind",
                      "Attempting port binding - retry enabled: " + std::to_string(cfg_.enable_port_retry) +
                          ", max retries: " + std::to_string(cfg_.max_port_retries) +
                          ", retry count: " + std::to_string(retry_count));
  }

  // Open acceptor (only if not already open)
  if (!acceptor_->is_open()) {
    acceptor_->open(tcp::v4(), ec);
    if (ec) {
      UNILINK_LOG_ERROR("tcp_server", "open", "Failed to open acceptor: " + ec.message());
      error_reporting::report_connection_error("tcp_server", "open", ec, false);
      state_.set_state(LinkState::Error);
      notify_state();
      return;
    }
  }

  // Bind to port
  acceptor_->bind(tcp::endpoint(tcp::v4(), cfg_.port), ec);
  if (ec) {
    // Check if retry is enabled and we haven't exceeded max retries
    if (cfg_.enable_port_retry && retry_count < cfg_.max_port_retries) {
      UNILINK_LOG_WARNING("tcp_server", "bind",
                          "Failed to bind to port " + std::to_string(cfg_.port) + " (attempt " +
                              std::to_string(retry_count + 1) + "/" + std::to_string(cfg_.max_port_retries) + "): " +
                              ec.message() + ". Retrying in " + std::to_string(cfg_.port_retry_interval_ms) + "ms...");

      // Schedule retry
      auto self = shared_from_this();
      auto timer = std::make_shared<net::steady_timer>(ioc_);
      timer->expires_after(std::chrono::milliseconds(cfg_.port_retry_interval_ms));
      timer->async_wait([self, retry_count, timer](const boost::system::error_code& timer_ec) {
        if (!timer_ec) {
          self->attempt_port_binding(retry_count + 1);
        }
      });
      return;
    } else {
      // No retry enabled or max retries exceeded
      std::string error_msg = "Failed to bind to port: " + std::to_string(cfg_.port) + " - " + ec.message();
      if (cfg_.enable_port_retry) {
        error_msg += " (after " + std::to_string(retry_count) + " retries)";
      }
      UNILINK_LOG_ERROR("tcp_server", "bind", error_msg);
      error_reporting::report_connection_error("tcp_server", "bind", ec, false);
      state_.set_state(LinkState::Error);
      notify_state();
      return;
    }
  }

  // Listen for connections
  acceptor_->listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    UNILINK_LOG_ERROR("tcp_server", "listen",
                      "Failed to listen on port: " + std::to_string(cfg_.port) + " - " + ec.message());
    error_reporting::report_connection_error("tcp_server", "listen", ec, false);
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  // Success
  if (retry_count > 0) {
    UNILINK_LOG_INFO("tcp_server", "bind",
                     "Successfully bound to port " + std::to_string(cfg_.port) + " after " +
                         std::to_string(retry_count) + " retries");
  } else {
    UNILINK_LOG_INFO("tcp_server", "bind", "Successfully bound to port " + std::to_string(cfg_.port));
  }

  state_.set_state(LinkState::Listening);
  notify_state();
  do_accept();
}

void TcpServer::do_accept() {
  if (!acceptor_ || !acceptor_->is_open()) return;

  auto self = shared_from_this();
  acceptor_->async_accept([self](auto ec, tcp::socket sock) {
    if (ec) {
      // "Operation canceled"는 정상적인 종료 과정에서 발생하는 에러이므로 로그 레벨을 낮춤
      if (ec == boost::asio::error::operation_aborted) {
        UNILINK_LOG_DEBUG("tcp_server", "accept", "Accept canceled (server shutting down)");
      } else {
        UNILINK_LOG_ERROR("tcp_server", "accept", "Accept error: " + ec.message());
        error_reporting::report_connection_error("tcp_server", "accept", ec, true);
        self->state_.set_state(LinkState::Error);
        self->notify_state();
      }
      // 서버가 종료 중이 아닌 경우에만 계속 수락
      if (!self->state_.is_state(LinkState::Closed)) {
        self->do_accept();
      }
      return;
    }

    boost::system::error_code ep_ec;
    auto rep = sock.remote_endpoint(ep_ec);
    std::string client_info = "unknown";
    if (!ep_ec) {
      client_info = rep.address().to_string() + ":" + std::to_string(rep.port());
    }

    // 클라이언트 제한 검사 (연결 수락 후, 세션 생성 전)
    if (self->client_limit_enabled_) {
      std::lock_guard<std::mutex> lock(self->sessions_mutex_);
      if (self->sessions_.size() >= self->max_clients_) {
        UNILINK_LOG_WARNING("tcp_server", "accept",
                            "Client connection rejected - server at capacity (" +
                                std::to_string(self->sessions_.size()) + "/" + std::to_string(self->max_clients_) +
                                "): " + client_info);

        // 소켓을 즉시 닫아서 연결 거부
        boost::system::error_code close_ec;
        sock.close(close_ec);
        if (close_ec) {
          UNILINK_LOG_DEBUG("tcp_server", "accept", "Error closing rejected socket: " + close_ec.message());
        }

        // 즉시 다음 연결 수락
        self->do_accept();
        return;
      }
    }

    if (!ep_ec) {
      UNILINK_LOG_INFO("tcp_server", "accept", "Client connected: " + client_info);
    } else {
      UNILINK_LOG_INFO("tcp_server", "accept", "Client connected (endpoint unknown)");
    }

    // 새 세션 생성
    auto new_session =
        std::make_shared<TcpServerSession>(self->ioc_, std::move(sock), self->cfg_.backpressure_threshold);

    // 세션을 리스트에 추가
    size_t client_id;
    {
      std::lock_guard<std::mutex> lock(self->sessions_mutex_);

      self->sessions_.push_back(new_session);
      client_id = self->sessions_.size() - 1;
    }

    // 현재 활성 세션 업데이트 (기존 API 호환성)
    self->current_session_ = new_session;

    // 세션 콜백 설정
    if (self->on_bytes_) {
      new_session->on_bytes([self, client_id](const uint8_t* data, size_t size) {
        // 기존 콜백 호출 (호환성)
        if (self->on_bytes_) {
          self->on_bytes_(data, size);
        }

        // 멀티 클라이언트 콜백 호출
        if (self->on_multi_data_) {
          std::string str_data = common::safe_convert::uint8_to_string(data, size);
          self->on_multi_data_(client_id, str_data);
        }
      });
    }

    if (self->on_bp_) new_session->on_backpressure(self->on_bp_);

    // 세션 종료 처리
    new_session->on_close([self, client_id, new_session] {
      // 멀티 클라이언트 콜백 호출
      if (self->on_multi_disconnect_) {
        self->on_multi_disconnect_(client_id);
      }

      // 세션 리스트에서 제거
      {
        std::lock_guard<std::mutex> lock(self->sessions_mutex_);
        auto it = std::find(self->sessions_.begin(), self->sessions_.end(), new_session);
        if (it != self->sessions_.end()) {
          self->sessions_.erase(it);
        }
      }

      // 현재 세션이 종료된 세션이면 정리
      if (self->current_session_ == new_session) {
        self->current_session_.reset();
        self->state_.set_state(LinkState::Listening);
        self->notify_state();
      }
    });

    // 멀티 클라이언트 연결 콜백 호출
    if (self->on_multi_connect_) {
      self->on_multi_connect_(client_id, client_info);
    }

    // 기존 API 호환성을 위한 상태 업데이트
    self->state_.set_state(LinkState::Connected);
    self->notify_state();

    new_session->start();

    // 즉시 다음 연결 수락 (멀티 클라이언트 지원)
    self->do_accept();
  });
}

void TcpServer::notify_state() {
  if (on_state_) {
    try {
      on_state_(state_.get_state());
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("tcp_server", "callback", "State callback error: " + std::string(e.what()));
      error_reporting::report_system_error("tcp_server", "state_callback",
                                           "Exception in state callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("tcp_server", "callback", "Unknown error in state callback");
      error_reporting::report_system_error("tcp_server", "state_callback", "Unknown error in state callback");
    }
  }
}

// 멀티 클라이언트 지원 메서드 구현
void TcpServer::broadcast(const std::string& message) {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  auto data = common::safe_convert::string_to_uint8(message);

  for (auto& session : sessions_) {
    if (session && session->alive()) {
      session->async_write_copy(data.data(), data.size());
    }
  }
}

void TcpServer::send_to_client(size_t client_id, const std::string& message) {
  std::lock_guard<std::mutex> lock(sessions_mutex_);

  if (client_id < sessions_.size() && sessions_[client_id] && sessions_[client_id]->alive()) {
    auto data = common::safe_convert::string_to_uint8(message);
    sessions_[client_id]->async_write_copy(data.data(), data.size());
  }
}

size_t TcpServer::get_client_count() const {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  return sessions_.size();
}

std::vector<size_t> TcpServer::get_connected_clients() const {
  std::lock_guard<std::mutex> lock(sessions_mutex_);
  std::vector<size_t> connected_clients;

  for (size_t i = 0; i < sessions_.size(); ++i) {
    if (sessions_[i] && sessions_[i]->alive()) {
      connected_clients.push_back(i);
    }
  }

  return connected_clients;
}

void TcpServer::on_multi_connect(MultiClientConnectHandler handler) { on_multi_connect_ = std::move(handler); }

void TcpServer::on_multi_data(MultiClientDataHandler handler) { on_multi_data_ = std::move(handler); }

void TcpServer::on_multi_disconnect(MultiClientDisconnectHandler handler) { on_multi_disconnect_ = std::move(handler); }

void TcpServer::set_client_limit(size_t max_clients) {
  max_clients_ = max_clients;
  client_limit_enabled_ = true;
}

void TcpServer::set_unlimited_clients() {
  client_limit_enabled_ = false;
  max_clients_ = 0;
}

}  // namespace transport
}  // namespace unilink
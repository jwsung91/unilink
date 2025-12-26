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

#include "unilink/transport/udp/udp.hpp"

#include <boost/system/error_code.hpp>
#include <stdexcept>

#include "unilink/common/common.hpp"
#include "unilink/common/error_handler.hpp"

namespace unilink {
namespace transport {

std::shared_ptr<UdpChannel> UdpChannel::create(const config::UdpConfig& cfg) {
  return std::shared_ptr<UdpChannel>(new UdpChannel(cfg));
}

std::shared_ptr<UdpChannel> UdpChannel::create(const config::UdpConfig& cfg, net::io_context& ioc) {
  return std::shared_ptr<UdpChannel>(new UdpChannel(cfg, ioc));
}

UdpChannel::UdpChannel(const config::UdpConfig& cfg)
    : owned_ioc_(std::make_unique<net::io_context>()),
      ioc_(owned_ioc_.get()),
      owns_ioc_(true),
      strand_(ioc_->get_executor()),
      socket_(strand_),
      cfg_(cfg),
      bp_high_(cfg.backpressure_threshold) {
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;
  bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
  if (bp_low_ == 0) bp_low_ = 1;
  bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                       common::constants::MAX_BUFFER_SIZE);
  if (bp_limit_ < bp_high_) {
    bp_limit_ = bp_high_;
  }
  set_remote_from_config();
}

UdpChannel::UdpChannel(const config::UdpConfig& cfg, net::io_context& ioc)
    : ioc_(&ioc),
      owns_ioc_(false),
      strand_(ioc.get_executor()),
      socket_(strand_),
      cfg_(cfg),
      bp_high_(cfg.backpressure_threshold) {
  cfg_.validate_and_clamp();
  bp_high_ = cfg_.backpressure_threshold;
  bp_low_ = bp_high_ > 1 ? bp_high_ / 2 : bp_high_;
  if (bp_low_ == 0) bp_low_ = 1;
  bp_limit_ = std::min(std::max(bp_high_ * 4, common::constants::DEFAULT_BACKPRESSURE_THRESHOLD),
                       common::constants::MAX_BUFFER_SIZE);
  if (bp_limit_ < bp_high_) {
    bp_limit_ = bp_high_;
  }
  set_remote_from_config();
}

UdpChannel::~UdpChannel() {
  if (started_ && !state_.is_state(LinkState::Closed)) {
    stop();
  }
}

void UdpChannel::start() {
  if (started_) return;
  if (!cfg_.is_valid()) {
    throw std::runtime_error("Invalid UDP configuration");
  }

  stopping_.store(false);
  if (owns_ioc_ && owned_ioc_ && owned_ioc_->stopped()) {
    owned_ioc_->restart();
  }

  work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(ioc_->get_executor());
  state_.set_state(LinkState::Connecting);
  notify_state();

  auto self = shared_from_this();
  net::post(strand_, [self]() { self->open_socket(); });

  if (owns_ioc_) {
    ioc_thread_ = std::thread([this]() {
      try {
        ioc_->run();
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("udp", "run", "Unhandled exception: " + std::string(e.what()));
      } catch (...) {
        UNILINK_LOG_ERROR("udp", "run", "Unknown unhandled exception");
      }
    });
  }

  started_ = true;
}

void UdpChannel::stop() {
  if (!started_) {
    state_.set_state(LinkState::Closed);
    return;
  }

  stopping_.store(true);
  if (work_guard_) work_guard_->reset();

  auto weak_self = weak_from_this();
  net::post(strand_, [weak_self]() {
    if (auto self = weak_self.lock()) {
      self->close_socket();
      self->tx_.clear();
      self->queue_bytes_ = 0;
      self->writing_ = false;
      self->report_backpressure(self->queue_bytes_);
    }
  });

  if (owns_ioc_) {
    if (ioc_) {
      ioc_->stop();
    }
    if (ioc_thread_.joinable()) {
      ioc_thread_.join();
    }
    if (owned_ioc_) {
      owned_ioc_->restart();
    }
  }

  connected_.store(false);
  opened_.store(false);
  state_.set_state(LinkState::Closed);
  notify_state();
  started_ = false;
}

bool UdpChannel::is_connected() const { return connected_.load(); }

void UdpChannel::async_write_copy(const uint8_t* data, size_t size) {
  if (!data || size == 0) return;
  if (stopping_.load() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) return;
  if (!remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("udp", "write", "Write size exceeds maximum allowed");
    return;
  }

  if (cfg_.enable_memory_pool && size <= 65536) {
    common::PooledBuffer pooled(size);
    if (pooled.valid()) {
      common::safe_memory::safe_memcpy(pooled.data(), data, size);
      net::post(strand_, [self = shared_from_this(), buf = std::move(pooled), size]() mutable {
        if (!self->enqueue_buffer(std::move(buf), size)) return;
        self->do_write();
      });
      return;
    }
  }

  std::vector<uint8_t> copy(data, data + size);
  net::post(strand_, [self = shared_from_this(), buf = std::move(copy), size]() mutable {
    if (!self->enqueue_buffer(std::move(buf), size)) return;
    self->do_write();
  });
}

void UdpChannel::async_write_move(std::vector<uint8_t>&& data) {
  auto size = data.size();
  if (size == 0) return;
  if (stopping_.load() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) return;
  if (!remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  net::post(strand_, [self = shared_from_this(), buf = std::move(data), size]() mutable {
    if (!self->enqueue_buffer(std::move(buf), size)) return;
    self->do_write();
  });
}

void UdpChannel::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (!data || data->empty()) return;
  if (stopping_.load() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) return;
  if (!remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  auto size = data->size();
  net::post(strand_, [self = shared_from_this(), buf = std::move(data), size]() mutable {
    if (!self->enqueue_buffer(std::move(buf), size)) return;
    self->do_write();
  });
}

void UdpChannel::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }

void UdpChannel::on_state(OnState cb) { on_state_ = std::move(cb); }

void UdpChannel::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }

void UdpChannel::open_socket() {
  boost::system::error_code ec;
  auto address = net::ip::make_address(cfg_.local_address, ec);
  if (ec) {
    UNILINK_LOG_ERROR("udp", "bind", "Invalid local address: " + cfg_.local_address);
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  local_endpoint_ = udp::endpoint(address, cfg_.local_port);
  socket_.open(local_endpoint_.protocol(), ec);
  if (ec) {
    UNILINK_LOG_ERROR("udp", "open", "Socket open failed: " + ec.message());
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  socket_.bind(local_endpoint_, ec);
  if (ec) {
    UNILINK_LOG_ERROR("udp", "bind", "Bind failed: " + ec.message());
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  opened_.store(true);
  if (remote_endpoint_) {
    connected_.store(true);
    state_.set_state(LinkState::Connected);
  } else {
    state_.set_state(LinkState::Listening);
  }
  notify_state();
  start_receive();
}

void UdpChannel::start_receive() {
  if (stopping_.load() || !socket_.is_open()) return;

  auto self = shared_from_this();
  socket_.async_receive_from(
      net::buffer(rx_), recv_endpoint_,
      [self](const boost::system::error_code& ec, std::size_t bytes) { self->handle_receive(ec, bytes); });
}

void UdpChannel::handle_receive(const boost::system::error_code& ec, std::size_t bytes) {
  if (stopping_.load() || ec == boost::asio::error::operation_aborted) return;

  // Fail-fast on truncation: treat both message_size and full-buffer reads as truncated datagrams.
  // We set Error once and exit without re-arming the receive loop, so user callbacks are not spammed.
  if (ec == boost::asio::error::message_size || bytes >= rx_.size()) {
    UNILINK_LOG_ERROR("udp", "receive", "Datagram truncated (buffer too small)");
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  if (ec) {
    UNILINK_LOG_ERROR("udp", "receive", "Receive failed: " + ec.message());
    state_.set_state(LinkState::Error);
    notify_state();
    return;
  }

  if (!remote_endpoint_) {
    remote_endpoint_ = recv_endpoint_;
    connected_.store(true);
    state_.set_state(LinkState::Connected);
    notify_state();
  }

  if (bytes > 0 && on_bytes_) {
    try {
      on_bytes_(rx_.data(), bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("udp", "on_bytes", "Exception in bytes callback: " + std::string(e.what()));
      if (cfg_.stop_on_callback_exception) {
        state_.set_state(LinkState::Error);
        notify_state();
        return;
      }
    } catch (...) {
      UNILINK_LOG_ERROR("udp", "on_bytes", "Unknown exception in bytes callback");
      if (cfg_.stop_on_callback_exception) {
        state_.set_state(LinkState::Error);
        notify_state();
        return;
      }
    }
  }

  start_receive();
}

void UdpChannel::do_write() {
  if (writing_ || tx_.empty()) return;
  if (!remote_endpoint_) {
    writing_ = false;
    return;
  }

  writing_ = true;

  auto current = std::move(tx_.front());
  auto bytes_queued = queued_bytes_front();

  auto on_write = [self = shared_from_this(), bytes_queued](const boost::system::error_code& ec, std::size_t) {
    self->queue_bytes_ = (self->queue_bytes_ > bytes_queued) ? (self->queue_bytes_ - bytes_queued) : 0;
    if (!self->tx_.empty()) self->tx_.pop_front();
    self->report_backpressure(self->queue_bytes_);

    if (ec) {
      if (ec != boost::asio::error::operation_aborted) {
        UNILINK_LOG_ERROR("udp", "write", "Send failed: " + ec.message());
        self->state_.set_state(LinkState::Error);
        self->notify_state();
      }
      self->writing_ = false;
      return;
    }

    self->writing_ = false;
    self->do_write();
  };

  if (std::holds_alternative<common::PooledBuffer>(current)) {
    auto pooled = std::get<common::PooledBuffer>(std::move(current));
    auto shared_buf = std::make_shared<common::PooledBuffer>(std::move(pooled));
    socket_.async_send_to(net::buffer(shared_buf->data(), shared_buf->size()), *remote_endpoint_,
                          [shared_buf, on_write = std::move(on_write)](
                              const boost::system::error_code& ec, std::size_t bytes) mutable { on_write(ec, bytes); });
  } else if (std::holds_alternative<std::shared_ptr<const std::vector<uint8_t>>>(current)) {
    auto shared_vec = std::get<std::shared_ptr<const std::vector<uint8_t>>>(std::move(current));
    socket_.async_send_to(net::buffer(shared_vec->data(), shared_vec->size()), *remote_endpoint_,
                          [shared_vec, on_write = std::move(on_write)](
                              const boost::system::error_code& ec, std::size_t bytes) mutable { on_write(ec, bytes); });
  } else {
    auto vec = std::get<std::vector<uint8_t>>(std::move(current));
    auto shared_vec = std::make_shared<std::vector<uint8_t>>(std::move(vec));
    socket_.async_send_to(net::buffer(shared_vec->data(), shared_vec->size()), *remote_endpoint_,
                          [shared_vec, on_write = std::move(on_write)](
                              const boost::system::error_code& ec, std::size_t bytes) mutable { on_write(ec, bytes); });
  }
}

void UdpChannel::close_socket() {
  boost::system::error_code ec;
  socket_.cancel(ec);
  socket_.close(ec);
}

void UdpChannel::notify_state() {
  if (!on_state_) return;
  auto current = state_.get_state();
  auto previous = last_notified_.exchange(current);
  if (previous == current) return;
  on_state_(current);
}

void UdpChannel::report_backpressure(size_t queued_bytes) {
  if (!on_bp_) return;

  if (!backpressure_active_ && queued_bytes >= bp_high_) {
    backpressure_active_ = true;
    try {
      on_bp_(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("udp", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("udp", "on_backpressure", "Unknown exception in backpressure callback");
    }
  } else if (backpressure_active_ && queued_bytes <= bp_low_) {
    backpressure_active_ = false;
    try {
      on_bp_(queued_bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("udp", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
    } catch (...) {
      UNILINK_LOG_ERROR("udp", "on_backpressure", "Unknown exception in backpressure callback");
    }
  }
}

size_t UdpChannel::queued_bytes_front() const {
  if (tx_.empty()) return 0;

  const auto& front = tx_.front();
  if (std::holds_alternative<common::PooledBuffer>(front)) {
    return std::get<common::PooledBuffer>(front).size();
  }
  if (std::holds_alternative<std::shared_ptr<const std::vector<uint8_t>>>(front)) {
    return std::get<std::shared_ptr<const std::vector<uint8_t>>>(front)->size();
  }
  return std::get<std::vector<uint8_t>>(front).size();
}

bool UdpChannel::enqueue_buffer(
    std::variant<common::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>&& buffer,
    size_t size) {
  if (queue_bytes_ + size > bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded");
    state_.set_state(LinkState::Error);
    notify_state();
    return false;
  }
  queue_bytes_ += size;
  tx_.push_back(std::move(buffer));
  report_backpressure(queue_bytes_);
  return true;
}

void UdpChannel::set_remote_from_config() {
  if (!cfg_.remote_address || !cfg_.remote_port) return;
  boost::system::error_code ec;
  auto addr = net::ip::make_address(*cfg_.remote_address, ec);
  if (ec) {
    throw std::runtime_error("Invalid remote address: " + *cfg_.remote_address);
  }
  remote_endpoint_ = udp::endpoint(addr, *cfg_.remote_port);
}

}  // namespace transport
}  // namespace unilink

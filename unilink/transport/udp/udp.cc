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
#include <type_traits>

#include "unilink/base/common.hpp"
#include "unilink/diagnostics/error_handler.hpp"

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
  stop();
  clear_callbacks();
  join_ioc_thread(true);
}

void UdpChannel::start() {
  if (started_) return;
  if (!cfg_.is_valid()) {
    throw std::runtime_error("Invalid UDP configuration");
  }

  if (owns_ioc_ && owned_ioc_ && owned_ioc_->stopped()) {
    owned_ioc_->restart();
  }

  if (ioc_thread_.joinable()) {
    join_ioc_thread(false);
  }

  if (owns_ioc_) {
    work_guard_ = std::make_unique<net::executor_work_guard<net::io_context::executor_type>>(ioc_->get_executor());
  }

  auto self = shared_from_this();
  net::dispatch(strand_, [self]() {
    self->reset_start_state();
    self->transition_to(LinkState::Connecting);
    self->open_socket();
  });

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
  if (stop_requested_.exchange(true)) return;

  if (!started_) {
    transition_to(LinkState::Closed);
    clear_callbacks();
    return;
  }

  stopping_.store(true);
  auto weak = weak_from_this();
  if (!ioc_) {
    perform_stop_cleanup();
  } else if (auto self = weak.lock()) {
    net::post(strand_, [self]() { self->perform_stop_cleanup(); });
  } else {
    perform_stop_cleanup();
  }

  join_ioc_thread(false);

  if (owns_ioc_ && owned_ioc_) {
    owned_ioc_->restart();
  }

  started_ = false;
}

bool UdpChannel::is_connected() const { return connected_.load(); }

void UdpChannel::async_write_copy(const uint8_t* data, size_t size) {
  if (!data || size == 0) return;
  if (stop_requested_.load()) return;
  if (stopping_.load() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) return;
  if (!remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  if (size > common::constants::MAX_BUFFER_SIZE) {
    UNILINK_LOG_ERROR("udp", "write", "Write size exceeds maximum allowed");
    return;
  }

  if (size > bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded by single write");
    transition_to(LinkState::Error);
    return;
  }

  if (cfg_.enable_memory_pool && size <= 65536) {
    memory::PooledBuffer pooled(size);
    if (pooled.valid()) {
      common::safe_memory::safe_memcpy(pooled.data(), data, size);
      net::post(strand_, [self = weak_from_this(), buf = std::move(pooled), size]() mutable {
        if (auto s = self.lock()) {
          if (!s->enqueue_buffer(std::move(buf), size)) return;
          s->do_write();
        }
      });
      return;
    }
  }

  std::vector<uint8_t> copy(data, data + size);
  net::post(strand_, [self = weak_from_this(), buf = std::move(copy), size]() mutable {
    if (auto s = self.lock()) {
      if (!s->enqueue_buffer(std::move(buf), size)) return;
      s->do_write();
    }
  });
}

void UdpChannel::async_write_move(std::vector<uint8_t>&& data) {
  auto size = data.size();
  if (size == 0) return;
  if (stop_requested_.load()) return;
  if (stopping_.load() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) return;
  if (!remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  if (size > bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded by single write");
    transition_to(LinkState::Error);
    return;
  }

  net::post(strand_, [self = weak_from_this(), buf = std::move(data), size]() mutable {
    if (auto s = self.lock()) {
      if (!s->enqueue_buffer(std::move(buf), size)) return;
      s->do_write();
    }
  });
}

void UdpChannel::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  if (!data || data->empty()) return;
  if (stop_requested_.load()) return;
  if (stopping_.load() || state_.is_state(LinkState::Closed) || state_.is_state(LinkState::Error)) return;
  if (!remote_endpoint_) {
    UNILINK_LOG_WARNING("udp", "write", "Remote endpoint not set; dropping write request");
    return;
  }

  auto size = data->size();
  if (size > bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded by single write");
    transition_to(LinkState::Error);
    return;
  }

  net::post(strand_, [self = weak_from_this(), buf = std::move(data), size]() mutable {
    if (auto s = self.lock()) {
      if (!s->enqueue_buffer(std::move(buf), size)) return;
      s->do_write();
    }
  });
}

void UdpChannel::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }

void UdpChannel::on_state(OnState cb) { on_state_ = std::move(cb); }

void UdpChannel::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }

void UdpChannel::open_socket() {
  if (stopping_.load() || stop_requested_.load()) return;

  boost::system::error_code ec;
  auto address = net::ip::make_address(cfg_.local_address, ec);
  if (ec) {
    UNILINK_LOG_ERROR("udp", "bind", "Invalid local address: " + cfg_.local_address);
    transition_to(LinkState::Error, ec);
    return;
  }

  local_endpoint_ = udp::endpoint(address, cfg_.local_port);
  socket_.open(local_endpoint_.protocol(), ec);
  if (ec) {
    UNILINK_LOG_ERROR("udp", "open", "Socket open failed: " + ec.message());
    transition_to(LinkState::Error, ec);
    return;
  }

  socket_.bind(local_endpoint_, ec);
  if (ec) {
    UNILINK_LOG_ERROR("udp", "bind", "Bind failed: " + ec.message());
    transition_to(LinkState::Error, ec);
    return;
  }

  opened_.store(true);
  if (remote_endpoint_) {
    connected_.store(true);
    transition_to(LinkState::Connected);
  } else {
    transition_to(LinkState::Listening);
  }
  start_receive();
}

void UdpChannel::start_receive() {
  if (stopping_.load() || stop_requested_.load() || state_.is_state(LinkState::Closed) ||
      state_.is_state(LinkState::Error) || !socket_.is_open()) {
    return;
  }

  auto self = shared_from_this();
  socket_.async_receive_from(
      net::buffer(rx_), recv_endpoint_,
      [self](const boost::system::error_code& ec, std::size_t bytes) { self->handle_receive(ec, bytes); });
}

void UdpChannel::handle_receive(const boost::system::error_code& ec, std::size_t bytes) {
  if (ec == boost::asio::error::operation_aborted) {
    return;
  }

  if (stopping_.load() || stop_requested_.load() || state_.is_state(LinkState::Closed) ||
      state_.is_state(LinkState::Error)) {
    return;
  }

  // Fail-fast on truncation: treat both message_size and full-buffer reads as truncated datagrams.
  // We set Error once and exit without re-arming the receive loop, so user callbacks are not spammed.
  if (ec == boost::asio::error::message_size || bytes >= rx_.size()) {
    UNILINK_LOG_ERROR("udp", "receive", "Datagram truncated (buffer too small)");
    transition_to(LinkState::Error, ec);
    return;
  }

  if (ec) {
    UNILINK_LOG_ERROR("udp", "receive", "Receive failed: " + ec.message());
    transition_to(LinkState::Error, ec);
    return;
  }

  if (!remote_endpoint_) {
    remote_endpoint_ = recv_endpoint_;
    connected_.store(true);
    transition_to(LinkState::Connected);
  }

  if (bytes > 0 && on_bytes_) {
    try {
      on_bytes_(rx_.data(), bytes);
    } catch (const std::exception& e) {
      UNILINK_LOG_ERROR("udp", "on_bytes", "Exception in bytes callback: " + std::string(e.what()));
      if (cfg_.stop_on_callback_exception) {
        transition_to(LinkState::Error);
        return;
      }
    } catch (...) {
      UNILINK_LOG_ERROR("udp", "on_bytes", "Unknown exception in bytes callback");
      if (cfg_.stop_on_callback_exception) {
        transition_to(LinkState::Error);
        return;
      }
    }
  }

  start_receive();
}

void UdpChannel::do_write() {
  if (writing_ || tx_.empty()) return;
  if (stop_requested_.load() || stopping_.load() || state_.is_state(LinkState::Closed) ||
      state_.is_state(LinkState::Error)) {
    tx_.clear();
    queue_bytes_ = 0;
    writing_ = false;
    backpressure_active_ = false;
    report_backpressure(queue_bytes_);
    return;
  }
  if (!remote_endpoint_) {
    writing_ = false;
    return;
  }

  writing_ = true;

  auto current = std::move(tx_.front());
  tx_.pop_front();
  auto bytes_queued = std::visit(
      [](auto&& buf) -> size_t {
        using Buffer = std::decay_t<decltype(buf)>;
        if constexpr (std::is_same_v<Buffer, std::shared_ptr<const std::vector<uint8_t>>>) {
          return buf ? buf->size() : 0;
        } else {
          return buf.size();
        }
      },
      current);

  auto on_write = [self = shared_from_this(), bytes_queued](const boost::system::error_code& ec, std::size_t) {
    self->queue_bytes_ = (self->queue_bytes_ > bytes_queued) ? (self->queue_bytes_ - bytes_queued) : 0;
    self->report_backpressure(self->queue_bytes_);

    if (ec == boost::asio::error::operation_aborted) {
      self->writing_ = false;
      return;
    }

    if (self->stop_requested_.load() || self->stopping_.load() || self->state_.is_state(LinkState::Closed) ||
        self->state_.is_state(LinkState::Error)) {
      self->writing_ = false;
      self->tx_.clear();
      self->queue_bytes_ = 0;
      self->report_backpressure(self->queue_bytes_);
      return;
    }

    if (ec) {
      UNILINK_LOG_ERROR("udp", "write", "Send failed: " + ec.message());
      self->transition_to(LinkState::Error, ec);
      self->writing_ = false;
      return;
    }

    self->writing_ = false;
    self->do_write();
  };

  std::visit(
      [&](auto&& buf) {
        using T = std::decay_t<decltype(buf)>;

        auto* data_ptr = [&]() {
          if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
            return buf->data();
          } else {
            return buf.data();
          }
        }();

        auto size = [&]() {
          if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>) {
            return buf->size();
          } else {
            return buf.size();
          }
        }();

        // Optimization: Move buffer directly into lambda to avoid additional allocation
        // 'buf' is an rvalue reference to the variant content here because we visit std::move(current)
        socket_.async_send_to(
            net::buffer(data_ptr, size), *remote_endpoint_,
            [buf_captured = std::move(buf), on_write = std::move(on_write)](
                const boost::system::error_code& ec, std::size_t bytes) mutable { on_write(ec, bytes); });
      },
      std::move(current));
}

void UdpChannel::close_socket() {
  boost::system::error_code ec;
  socket_.cancel(ec);
  socket_.close(ec);
}

void UdpChannel::notify_state() {
  if (!on_state_) return;
  try {
    on_state_(state_.get_state());
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("udp", "on_state", "Exception in state callback: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("udp", "on_state", "Unknown exception in state callback");
  }
}

void UdpChannel::report_backpressure(size_t queued_bytes) {
  if (stop_requested_.load() || !on_bp_) return;

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
  if (std::holds_alternative<memory::PooledBuffer>(front)) {
    return std::get<memory::PooledBuffer>(front).size();
  }
  if (std::holds_alternative<std::shared_ptr<const std::vector<uint8_t>>>(front)) {
    return std::get<std::shared_ptr<const std::vector<uint8_t>>>(front)->size();
  }
  return std::get<std::vector<uint8_t>>(front).size();
}

bool UdpChannel::enqueue_buffer(
    std::variant<memory::PooledBuffer, std::vector<uint8_t>, std::shared_ptr<const std::vector<uint8_t>>>&& buffer,
    size_t size) {
  if (stopping_.load() || stop_requested_.load() || state_.is_state(LinkState::Closed) ||
      state_.is_state(LinkState::Error)) {
    return false;
  }

  if (queue_bytes_ + size > bp_limit_) {
    UNILINK_LOG_ERROR("udp", "write", "Queue limit exceeded");
    transition_to(LinkState::Error);
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

void UdpChannel::transition_to(LinkState target, const boost::system::error_code& ec) {
  if (ec == net::error::operation_aborted) {
    return;
  }

  const auto current = state_.get_state();
  if ((current == LinkState::Closed || current == LinkState::Error) &&
      (target == LinkState::Closed || target == LinkState::Error)) {
    return;
  }

  if (target == LinkState::Closed || target == LinkState::Error) {
    if (terminal_state_notified_.exchange(true)) {
      return;
    }
  } else if (current == target) {
    return;
  }

  state_.set_state(target);
  notify_state();
}

void UdpChannel::clear_callbacks() {
  on_bytes_ = nullptr;
  on_state_ = nullptr;
  on_bp_ = nullptr;
}

void UdpChannel::perform_stop_cleanup() {
  try {
    close_socket();
    tx_.clear();
    queue_bytes_ = 0;
    writing_ = false;
    const bool had_backpressure = backpressure_active_;
    backpressure_active_ = false;
    if (had_backpressure && on_bp_) {
      try {
        on_bp_(queue_bytes_);
      } catch (const std::exception& e) {
        UNILINK_LOG_ERROR("udp", "on_backpressure", "Exception in backpressure callback: " + std::string(e.what()));
      } catch (...) {
        UNILINK_LOG_ERROR("udp", "on_backpressure", "Unknown exception in backpressure callback");
      }
    }
    connected_.store(false);
    opened_.store(false);
    if (owns_ioc_ && ioc_) {
      ioc_->stop();
    }
    if (owns_ioc_ && work_guard_) {
      work_guard_->reset();
    }
    transition_to(LinkState::Closed);
    clear_callbacks();
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("udp", "stop_cleanup", "Cleanup error: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("udp", "stop_cleanup", "Unknown error in stop cleanup");
  }
}

void UdpChannel::reset_start_state() {
  stop_requested_.store(false);
  stopping_.store(false);
  terminal_state_notified_.store(false);
  connected_.store(false);
  opened_.store(false);
  writing_ = false;
  queue_bytes_ = 0;
  backpressure_active_ = false;
  state_.set_state(LinkState::Idle);
}

void UdpChannel::join_ioc_thread(bool allow_detach) {
  if (!owns_ioc_ || !ioc_thread_.joinable()) {
    return;
  }

  if (std::this_thread::get_id() == ioc_thread_.get_id()) {
    if (allow_detach) {
      ioc_thread_.detach();
    }
    return;
  }

  try {
    ioc_thread_.join();
  } catch (const std::exception& e) {
    UNILINK_LOG_ERROR("udp", "join", "Join failed: " + std::string(e.what()));
  } catch (...) {
    UNILINK_LOG_ERROR("udp", "join", "Join failed with unknown error");
  }
}

}  // namespace transport
}  // namespace unilink

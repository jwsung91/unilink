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

#include "unilink/transport/uds/uds_server_session.hpp"

#include "unilink/transport/uds/boost_uds_socket.hpp"

namespace unilink {
namespace transport {

UdsServerSession::UdsServerSession(net::io_context& ioc, uds::socket sock, size_t backpressure_threshold)
    : ioc_(ioc),
      strand_(net::make_strand(ioc_)),
      socket_(std::make_unique<BoostUdsSocket>(std::move(sock))),
      bp_high_(backpressure_threshold),
      bp_limit_(backpressure_threshold * 2) {}

UdsServerSession::UdsServerSession(net::io_context& ioc, std::unique_ptr<interface::UdsSocketInterface> socket,
                                   size_t backpressure_threshold)
    : ioc_(ioc),
      strand_(net::make_strand(ioc_)),
      socket_(std::move(socket)),
      bp_high_(backpressure_threshold),
      bp_limit_(backpressure_threshold * 2) {}

void UdsServerSession::start() {
  alive_ = true;
  start_read();
}

void UdsServerSession::stop() {
  if (closing_.exchange(true)) return;
  net::post(strand_, [this, self = shared_from_this()]() {
    on_bytes_ = nullptr;
    on_bp_ = nullptr;
    do_close();
  });
}

bool UdsServerSession::alive() const { return alive_.load(); }

void UdsServerSession::async_write_copy(memory::ConstByteSpan data) {
  std::vector<uint8_t> vec(data.begin(), data.end());
  async_write_move(std::move(vec));
}

void UdsServerSession::async_write_move(std::vector<uint8_t>&& data) {
  net::post(strand_, [this, self = shared_from_this(), data = std::move(data)]() mutable {
    if (!alive_) return;
    size_t added = data.size();
    if (queue_bytes_ + added > bp_limit_) return;

    queue_bytes_ += added;
    tx_.emplace_back(std::move(data));
    report_backpressure(queue_bytes_);
    if (!writing_) do_write();
  });
}

void UdsServerSession::async_write_shared(std::shared_ptr<const std::vector<uint8_t>> data) {
  net::post(strand_, [this, self = shared_from_this(), data = std::move(data)]() {
    if (!alive_) return;
    size_t added = data->size();
    if (queue_bytes_ + added > bp_limit_) return;

    queue_bytes_ += added;
    tx_.emplace_back(std::move(data));
    report_backpressure(queue_bytes_);
    if (!writing_) do_write();
  });
}

void UdsServerSession::on_bytes(OnBytes cb) { on_bytes_ = std::move(cb); }
void UdsServerSession::on_backpressure(OnBackpressure cb) { on_bp_ = std::move(cb); }
void UdsServerSession::on_close(OnClose cb) { on_close_ = std::move(cb); }

void UdsServerSession::start_read() {
  socket_->async_read_some(
      net::buffer(rx_),
      net::bind_executor(strand_, [this, self = shared_from_this()](const boost::system::error_code& ec, size_t bytes) {
        if (closing_ || !alive_) return;
        if (ec) {
          do_close();
          return;
        }
        if (on_bytes_) on_bytes_(memory::ConstByteSpan(rx_.data(), bytes));
        start_read();
      }));
}

void UdsServerSession::do_write() {
  if (tx_.empty() || writing_) return;
  writing_ = true;
  current_write_buffer_ = std::move(tx_.front());
  tx_.pop_front();

  net::const_buffer buffer;
  std::visit(
      [&buffer](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::vector<uint8_t>>)
          buffer = net::buffer(arg);
        else if constexpr (std::is_same_v<T, std::shared_ptr<const std::vector<uint8_t>>>)
          buffer = net::buffer(*arg);
        else if constexpr (std::is_same_v<T, memory::PooledBuffer>)
          buffer = net::buffer(arg.data(), arg.size());
      },
      *current_write_buffer_);

  size_t bytes_to_write = buffer.size();
  socket_->async_write(buffer, net::bind_executor(strand_, [this, self = shared_from_this(), bytes_to_write](
                                                               const boost::system::error_code& ec, size_t) {
                         if (closing_ || !alive_) return;
                         writing_ = false;
                         current_write_buffer_ = std::nullopt;
                         queue_bytes_ = (queue_bytes_ >= bytes_to_write) ? (queue_bytes_ - bytes_to_write) : 0;
                         report_backpressure(queue_bytes_);

                         if (ec) {
                           do_close();
                           return;
                         }
                         if (!tx_.empty()) do_write();
                       }));
}

void UdsServerSession::do_close() {
  if (!closing_.exchange(true) && !alive_) return;
  alive_ = false;
  auto close_cb = std::move(on_close_);
  on_bytes_ = nullptr;
  on_bp_ = nullptr;
  on_close_ = nullptr;
  tx_.clear();
  current_write_buffer_ = std::nullopt;
  queue_bytes_ = 0;
  writing_ = false;
  boost::system::error_code ec;
  socket_->close(ec);
  if (close_cb) {
    try {
      close_cb();
    } catch (...) {
    }
  }
}

void UdsServerSession::report_backpressure(size_t queued_bytes) {
  if (closing_ || !alive_) return;
  if (on_bp_) on_bp_(queued_bytes);
}

}  // namespace transport
}  // namespace unilink

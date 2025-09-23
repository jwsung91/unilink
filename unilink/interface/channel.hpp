#pragma once
#include <functional>

#include "unilink/common/common.hpp"

namespace unilink {
namespace interface {
class Channel {
 public:
  using OnBytes = std::function<void(const uint8_t*, size_t)>;
  using OnState = std::function<void(common::LinkState)>;
  using OnBackpressure = std::function<void(size_t /*queued_bytes*/)>;

  virtual ~Channel() = default;

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool is_connected() const = 0;

  // Single send API (copies into internal queue)
  virtual void async_write_copy(const uint8_t* data, size_t size) = 0;

  // Callbacks
  virtual void on_bytes(OnBytes cb) = 0;
  virtual void on_state(OnState cb) = 0;
  virtual void on_backpressure(OnBackpressure cb) = 0;
};
}  // namespace interface
}  // namespace unilink
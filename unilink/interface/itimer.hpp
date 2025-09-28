#pragma once

#include <boost/asio.hpp>
#include <functional>

namespace unilink {
namespace interface {

namespace net = boost::asio;

/**
 * @brief An interface abstracting Boost.Asio's steady_timer for testability.
 * This is an internal interface used for dependency injection and mocking.
 */
class TimerInterface {
 public:
  virtual ~TimerInterface() = default;

  virtual void expires_after(std::chrono::milliseconds expiry_time) = 0;
  virtual void async_wait(std::function<void(const boost::system::error_code&)> handler) = 0;
  virtual void cancel() = 0;
};

}  // namespace interface
}  // namespace unilink

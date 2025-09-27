#pragma once

#include <boost/asio.hpp>
#include <functional>

namespace unilink {
namespace interface {

namespace net = boost::asio;

/**
 * @brief An interface abstracting Boost.Asio's tcp::resolver for testability.
 * This is an internal interface used for dependency injection and mocking.
 */
class ITcpResolver {
 public:
  virtual ~ITcpResolver() = default;

  virtual void async_resolve(
      const std::string& host,
      const std::string& service,
      std::function<void(const boost::system::error_code&, net::ip::tcp::resolver::results_type)> handler) = 0;
};

}  // namespace interface
}  // namespace unilink

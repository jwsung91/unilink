#include "unilink/unilink.hpp"

#include "unilink/factory/channel_factory.hpp"

namespace unilink {

std::shared_ptr<IChannel> create(const ChannelOptions& options) {
  return factory::ChannelFactory::create(options);
}

}  // namespace unilink
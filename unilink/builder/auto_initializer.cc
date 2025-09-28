#include "unilink/builder/auto_initializer.hpp"

namespace unilink {
namespace builder {

std::mutex AutoInitializer::init_mutex_;
std::atomic<bool> AutoInitializer::initialized_{false};

} // namespace builder
} // namespace unilink

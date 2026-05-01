#include <iostream>
#include <memory>

#include "unilink/unilink.hpp"

// We need to access protected members of BuilderInterface to verify the threshold.
// A simple way is to create a subclass.
template <typename T, typename Derived>
class VerifierBuilder : public unilink::builder::BuilderInterface<T, Derived> {
 public:
  size_t get_threshold() const { return this->get_effective_backpressure_threshold(); }
};

class TcpClientVerifier : public VerifierBuilder<unilink::wrapper::TcpClient, TcpClientVerifier> {
 public:
  std::unique_ptr<unilink::wrapper::TcpClient> build() override { return nullptr; }
  TcpClientVerifier& auto_start(bool) override { return *this; }
};

int main() {
  std::cout << "--- Verifying Dynamic Defaults (C++ Builder) ---" << std::endl;

  // 1. Reliable (Default)
  TcpClientVerifier builder_r;
  std::cout << "Reliable Default Threshold: " << (builder_r.get_threshold() / 1024.0 / 1024.0) << " MiB" << std::endl;

  // 2. BestEffort
  TcpClientVerifier builder_be;
  builder_be.backpressure_strategy(unilink::base::constants::BackpressureStrategy::BestEffort);
  std::cout << "BestEffort Default Threshold: " << (builder_be.get_threshold() / 1024.0) << " KiB" << std::endl;

  // 3. Explicitly set Reliable to 10MB
  TcpClientVerifier builder_explicit;
  builder_explicit.backpressure_threshold(10 * 1024 * 1024);
  std::cout << "Explicit (10MB) Threshold: " << (builder_explicit.get_threshold() / 1024.0 / 1024.0) << " MiB"
            << std::endl;

  return 0;
}

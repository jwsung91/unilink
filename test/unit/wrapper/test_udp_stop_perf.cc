#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "unilink/base/common.hpp"
#include "unilink/config/udp_config.hpp"
#include "unilink/memory/safe_span.hpp"
#include "unilink/wrapper/udp/udp.hpp"

using namespace unilink;
using namespace std::chrono_literals;

TEST(UdpWrapperTest, StopPerformance) {
  config::UdpConfig cfg;
  cfg.local_port = 0;
  cfg.remote_address = "127.0.0.1";
  cfg.remote_port = 19001;

  wrapper::Udp udp(cfg);
  udp.start();

  auto start = std::chrono::high_resolution_clock::now();
  udp.stop();
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "[PERF] Stop duration: " << duration.count() << "ms" << std::endl;
}

TEST(UdpWrapperTest, StopSafetyWithExternalIOC) {
  auto ioc = std::make_shared<boost::asio::io_context>();
  auto work_guard = boost::asio::make_work_guard(*ioc);
  std::thread io_thread([ioc] { ioc->run(); });

  config::UdpConfig cfg;
  cfg.local_port = 0;

  {
    wrapper::Udp udp(cfg, ioc);
    std::atomic<int> callbacks{0};
    udp.on_data([&](const wrapper::MessageContext&) { callbacks++; });
    udp.start();

    // Stop and destroy immediately
    udp.stop();
  }

  // Wait a bit to ensure no late callbacks cause crashes
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  work_guard.reset();
  ioc->stop();
  if (io_thread.joinable()) io_thread.join();
}

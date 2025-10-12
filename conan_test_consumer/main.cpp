#include <chrono>
#include <iostream>
#include <thread>
#include <unilink/unilink.hpp>

int main() {
  std::cout << "Testing unilink package from Conan..." << std::endl;

  try {
    // Test basic functionality
    auto client = unilink::tcp_client("127.0.0.1", 8080)
                      .on_connect([]() { std::cout << "Connected successfully!" << std::endl; })
                      .on_data([](const std::string& data) { std::cout << "Received: " << data << std::endl; })
                      .on_error([](const std::string& error) { std::cout << "Error: " << error << std::endl; })
                      .build();

    std::cout << "unilink client created successfully!" << std::endl;
    std::cout << "Conan package integration test passed!" << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

#include <future>
#include <iostream>

#include "unilink/unilink.hpp"

int main(int argc, char** argv) {
  unsigned short port =
      (argc > 1) ? static_cast<unsigned short>(std::stoi(argv[1])) : 9000;

  // Using builder pattern for configuration
  std::unique_ptr<unilink::wrapper::TcpServer> ul;
  
  ul = unilink::builder::UnifiedBuilder::tcp_server(port)
      .auto_start(false)
      .on_connect([&]() {
          unilink::log_message("[server]", "STATE", "Client connected");
      })
      .on_disconnect([&]() {
          unilink::log_message("[server]", "STATE", "Client disconnected");
      })
      .on_data([&](const std::string& data) {
          unilink::log_message("[server]", "RX", data);
          unilink::log_message("[server]", "TX", data);
          ul->send(data);
      })
      .build();

  ul->start();

  // 프로그램이 Ctrl+C로 종료될 때까지 무한정 대기합니다.
  std::promise<void>().get_future().wait();

  ul->stop();
  return 0;
}

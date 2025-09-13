// src/main.cc
#include <boost/asio.hpp>
#include <iostream>
#include <string>

#include "common.hpp"
#include "config.hpp"
#include "factory.hpp"
#include "ichannel.hpp"

static bool has_yaml_suffix(const std::string& s) {
  auto pos = s.rfind('.');
  if (pos == std::string::npos) return false;
  auto ext = s.substr(pos + 1);
  for (auto& c : ext) c = std::tolower(c);
  return (ext == "yml" || ext == "yaml");
}

int main(int argc, char** argv) {
  // 기본값
  std::string mode = "server";
  std::string host = "127.0.0.1";
  unsigned short port = 9000;

#ifdef INTERFACE_SOCKET_ENABLE_YAML
  if (argc > 1 && has_yaml_suffix(argv[1])) {
    auto cfg = load_config_from_yaml(argv[1]);
    mode = cfg.mode;
    host = cfg.host;
    port = cfg.port;
  } else
#endif
  {
    // CLI: interface_socket [server|client] [host] [port]
    if (argc > 1) mode = argv[1];
    if (argc > 2) host = argv[2];
    if (argc > 3) port = static_cast<unsigned short>(std::stoi(argv[3]));
  }

  boost::asio::io_context ioc;

  std::shared_ptr<IChannel> ch;
  if (mode == "server") {
    ch = make_server_single(ioc, port);
    ch->on_receive([&](const Msg& m) { ch->async_send(m); });  // echo
    ch->on_state(
        [](LinkState s) { std::cout << "[server] state=" << (int)s << "\n"; });
    std::cout << "[server] listening on " << port << "\n";
  } else {
    ch = make_client_single(ioc, host, port);
    ch->on_state(
        [](LinkState s) { std::cout << "[client] state=" << (int)s << "\n"; });
    std::cout << "[client] connecting to " << host << ":" << port << "\n";
  }

  ch->start();

  // 데모: client일 때 몇 개 request 날려보기
  if (mode == "client") {
    static bool sent = false;
    ch->on_state([&](LinkState s) {
      std::cout << "[client] state=" << (int)s << "\n";
      if (s == LinkState::Connected && !sent) {
        sent = true;

        // 아주 짧게 쉬고 전송 (선택)
        auto t = std::make_shared<boost::asio::steady_timer>(ioc);
        t->expires_after(std::chrono::milliseconds(50));
        t->async_wait([&, t](auto) {
          for (int i = 0; i < 3; ++i) {
            Msg m;
            auto payload = std::string("Hello ") + std::to_string(i);
            m.bytes.assign(payload.begin(), payload.end());

            // 1) request는 보낸 뒤
            auto fut = ch->request(m, std::chrono::milliseconds(1500));

            // 2) get()은 별도 스레드에서 — IO 스레드를 막지 않음
            std::thread([f = std::move(fut)]() mutable {
              try {
                auto resp = f.get();
                std::string s(resp.bytes.begin(), resp.bytes.end());
                std::cout << "[client] response seq=" << resp.seq << ": " << s
                          << std::endl;
              } catch (const std::exception& e) {
                std::cout << "[client] request error: " << e.what()
                          << std::endl;
              }
            }).detach();
          }
        });
      }
    });
  }

  ioc.run();
  return 0;
}

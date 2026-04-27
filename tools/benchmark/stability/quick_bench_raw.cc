#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std::chrono;

void run_raw(int duration_s) {
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10011);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(lfd, 1);

    std::atomic<uint64_t> recv_bytes{0};
    std::atomic<bool> running{true};

    std::thread server([&] {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);
        int cfd = accept(lfd, (struct sockaddr*)&cli, &len);
        std::vector<char> buf(65536);
        while (running) {
            ssize_t n = recv(cfd, buf.data(), buf.size(), 0);
            if (n <= 0) break;
            recv_bytes += n;
        }
        close(cfd);
    });

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in srv_addr{};
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(10011);
    inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);
    connect(cfd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));

    std::atomic<uint64_t> sent_bytes{0};
    std::string payload(65536, 'A');

    std::thread client_thread([&] {
        while (running) {
            ssize_t n = send(cfd, payload.data(), payload.size(), 0);
            if (n > 0) sent_bytes += n;
            else break;
        }
    });

    auto start = steady_clock::now();
    std::this_thread::sleep_for(seconds(duration_s));
    running = false;
    
    shutdown(cfd, SHUT_RDWR);
    close(cfd);
    client_thread.join();
    server.join();
    close(lfd);

    auto end = steady_clock::now();
    double elapsed = duration_cast<duration<double>>(end - start).count();

    std::cout << "--- Raw Socket ---\n"
              << "Sent: " << sent_bytes / (1024.0*1024.0) << " MB\n"
              << "Recv: " << recv_bytes / (1024.0*1024.0) << " MB\n"
              << "Throughput: " << (sent_bytes / (1024.0*1024.0)) / elapsed << " MB/s\n";
}

int main() {
    run_raw(10);
    return 0;
}

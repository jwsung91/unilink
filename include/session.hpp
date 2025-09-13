#pragma once
#include <boost/asio.hpp>
#include <deque>
#include <array>
#include <memory>
#include "common.hpp"
#include "inflight_table.hpp"

namespace net = boost::asio;
using tcp = net::ip::tcp;

// Frame: [uint16 len_be][uint32 seq_be][payload...]
// len includes the 4-byte seq.

class Session : public std::enable_shared_from_this<Session> {
public:
    using OnReceive = std::function<void(const Msg&)>;
    using OnClose   = std::function<void()>;

    Session(net::io_context& ioc, tcp::socket sock, OnReceive on_rx, OnClose on_close)
        : ioc_(ioc), socket_(std::move(sock)), strand_(socket_.get_executor()),
          on_rx_(std::move(on_rx)), on_close_(std::move(on_close)),
          sweep_timer_(ioc) {}

    void start() {
        set_alive(true);
        start_read_header();
        start_sweeper();
    }

    void close() {
        net::dispatch(strand_, [self=shared_from_this()](){ self->do_close(); });
    }

    bool alive() const { return alive_; }

    void send(Msg m) {
        net::dispatch(strand_, [self=shared_from_this(), m=std::move(m)]() mutable {
            self->enqueue_write(std::move(m));
        });
    }

    std::future<Msg> request(Msg m, std::chrono::milliseconds timeout) {
        auto seq = inflight_.next_seq();
        m.seq = seq;
        std::promise<Msg> prom;
        auto fut = prom.get_future();
        InflightTable::Pending p{std::move(prom), Clock::now() + timeout};
        net::dispatch(strand_, [self=shared_from_this(), seq, p=std::move(p), m=std::move(m)]() mutable {
            self->inflight_.emplace(seq, std::move(p));
            self->enqueue_write(std::move(m));
        });
        return fut;
    }

private:
    void enqueue_write(Msg m) {
        tx_bufs_.emplace_back(build_frame(m));
        if (!writing_) do_write();
    }

    std::vector<uint8_t> build_frame(const Msg& m) {
        const uint16_t len = static_cast<uint16_t>(4 + m.bytes.size());
        std::vector<uint8_t> f; f.reserve(2 + 4 + m.bytes.size());
        f.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        f.push_back(static_cast<uint8_t>(len & 0xFF));
        f.push_back(static_cast<uint8_t>((m.seq >> 24) & 0xFF));
        f.push_back(static_cast<uint8_t>((m.seq >> 16) & 0xFF));
        f.push_back(static_cast<uint8_t>((m.seq >> 8) & 0xFF));
        f.push_back(static_cast<uint8_t>(m.seq & 0xFF));
        f.insert(f.end(), m.bytes.begin(), m.bytes.end());
        return f;
    }

    void do_write() {
        writing_ = true;
        auto self = shared_from_this();
        net::async_write(socket_, net::buffer(tx_bufs_.front()),
            net::bind_executor(strand_, [self](auto ec, std::size_t){
                if (ec) { self->do_close(); return; }
                self->tx_bufs_.pop_front();
                if (!self->tx_bufs_.empty()) self->do_write();
                else self->writing_ = false;
            }));
    }

    void start_read_header() {
        auto self = shared_from_this();
        net::async_read(socket_, net::buffer(rx_hdr_),
            net::bind_executor(strand_, [self](auto ec, std::size_t){
                if (ec) { self->do_close(); return; }
                uint16_t len = (static_cast<uint16_t>(self->rx_hdr_[0]) << 8) | self->rx_hdr_[1];
                self->rx_body_.resize(len);
                self->start_read_body(len);
            }));
    }

    void start_read_body(std::size_t len) {
        auto self = shared_from_this();
        net::async_read(socket_, net::buffer(rx_body_.data(), len),
            net::bind_executor(strand_, [self](auto ec, std::size_t n){
                if (ec || n < 4) { self->do_close(); return; }
                uint32_t seq = (static_cast<uint32_t>(self->rx_body_[0]) << 24) |
                               (static_cast<uint32_t>(self->rx_body_[1]) << 16) |
                               (static_cast<uint32_t>(self->rx_body_[2]) << 8)  |
                               (static_cast<uint32_t>(self->rx_body_[3]));
                Msg m; m.seq = seq; m.bytes.assign(self->rx_body_.begin()+4, self->rx_body_.end());
                if (!self->inflight_.fulfill(seq, m)) {
                    if (self->on_rx_) self->on_rx_(m);
                }
                self->start_read_header();
            }));
    }

    void start_sweeper() {
        auto self = shared_from_this();
        sweep_timer_.expires_after(std::chrono::milliseconds(100));
        sweep_timer_.async_wait(net::bind_executor(strand_, [self](auto ec){
            if (ec) return;
            self->inflight_.sweep([&](uint32_t /*seq*/){ });
            if (self->alive_) self->start_sweeper();
        }));
    }

    void do_close() {
        if (!alive_) return;
        alive_ = false;
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        sweep_timer_.cancel();
        inflight_.clear_exception(std::make_exception_ptr(std::runtime_error("channel closed")));
        if (on_close_) on_close_();
    }

    void set_alive(bool v) { alive_ = v; }

private:
    net::io_context& ioc_;
    tcp::socket socket_;
    net::strand<tcp::socket::executor_type> strand_;

    std::array<uint8_t,2> rx_hdr_{};
    std::vector<uint8_t> rx_body_;

    std::deque<std::vector<uint8_t>> tx_bufs_;
    bool writing_ = false;

    InflightTable inflight_;
    net::steady_timer sweep_timer_;

    OnReceive on_rx_;
    OnClose   on_close_;
    bool alive_ = false;
};

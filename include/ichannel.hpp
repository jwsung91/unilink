#pragma once
#include <future>
#include <chrono>
#include "common.hpp"

class IChannel {
public:
    using OnReceive = std::function<void(const Msg&)>;
    using OnState   = std::function<void(LinkState)>;
    virtual ~IChannel() = default;

    virtual void start() = 0;     // client: connect, server: accept
    virtual void stop() = 0;      // graceful shutdown

    virtual bool is_connected() const = 0;
    virtual LinkState state() const = 0;

    virtual void async_send(Msg m) = 0;
    virtual std::future<Msg> request(Msg m, std::chrono::milliseconds timeout) = 0;

    virtual void on_receive(OnReceive cb) = 0;
    virtual void on_state(OnState cb) = 0;
};

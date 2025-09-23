# 📘 unilink API Guide

`unilink` is a unified, low-level asynchronous communication library. It provides a simple, consistent interface (`IChannel`) for various transport types, such as TCP (client/server) and Serial ports.

This guide explains how to use the library in your application.

---

## 1. Core Components

- **`unilink::create()`**: A factory function to create a communication channel. It takes a configuration struct and returns a `std::shared_ptr<IChannel>`.
- **`unilink::IChannel`**: The main interface for interacting with the channel. Key methods are `start()`, `stop()`, and `async_write_copy()`.
- **Configuration Structs**: Plain structs (`TcpClientConfig`, `TcpServerConfig`, `SerialConfig`) used to configure channel behavior, such as address, port, and retry logic.
- **Callbacks**: `std::function` objects (`on_bytes`, `on_state`) set on an `IChannel` instance to receive data and state change events asynchronously.

---

## 2. Initialization & Lifecycle

The library manages its own background thread for I/O, so you don't need to handle `boost::asio::io_context`.

```cpp
#include "unilink/unilink.hpp"
#include <thread>
#include <chrono>

using namespace unilink;
using namespace std::chrono_literals;

int main() {
    // 1. Configure the channel
    TcpClientConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 9000;
    cfg.retry_interval_ms = 2000; // Reconnect every 2s on failure

    // 2. Create the channel
    auto cli = unilink::create(cfg);

    // 3. Set callbacks for events
    cli->on_state( {
        log_message("[client]", "STATE", to_cstr(s));
    });
    cli->on_bytes( {
        // Handle received data
    });

    // 4. Start the channel (starts background I/O thread)
    cli->start();

    // ... Application logic ...
    std::this_thread::sleep_for(10s);

    // 5. Stop the channel and clean up
    cli->stop();

    return 0;
}
```

---

## 3. Connection State & Reconnection

Connection status is delivered asynchronously via the `on_state` callback.

- **`LinkState`**: An enum class representing the channel's state (`Idle`, `Connecting`, `Listening`, `Connected`, `Closed`, `Error`).
- **Reconnection**: For client and serial channels, you can enable automatic reconnection by setting `retry_interval_ms` in the config struct. If the connection is lost, the channel will enter the `Connecting` state and attempt to reconnect periodically.

---

## 4. Sending & Receiving Data

### Send

The `async_write_copy` method is thread-safe. It copies the data into an internal queue and sends it on the I/O thread.

```cpp
std::string msg = "hello\n";
cli->async_write_copy(
    reinterpret_cast<const uint8_t*>(msg.data()),
    msg.size()
);
```

### Receive via Callback

```cpp
class MyHandler : public ICommHandler {
public:
  void on_open() override        { /* connected */ }
  void on_close() override       { /* disconnected */ }
  void on_write(size_t n) override { /* bytes sent */ }
  void on_error(std::error_code ec, std::string_view where) override { /* logging */ }

  void on_receive(const uint8_t* data, size_t len) override {
      // Data valid only during callback → copy if needed
      buffer_.append(reinterpret_cast<const char*>(data), len);
      parse_frames();
  }
private:
  std::string buffer_;
  void parse_frames() {
      size_t pos;
      while ((pos = buffer_.find('\n')) != std::string::npos) {
          std::string frame = buffer_.substr(0, pos);
          buffer_.erase(0, pos + 1);
          handle_frame(frame);
      }
  }
  void handle_frame(const std::string& f) {
      std::cout << "[frame] " << f << "\n";
  }
};
```

---

## 4) Threading Model

- All I/O callbacks run **inside a strand**, ensuring **order and no race conditions**.
- Upper app may block on `std::future::wait()` safely → I/O still runs in the networking thread.
- If the app needs to consume data in another thread, use a **thread-safe queue** inside `on_receive`.

---

## 5) Error Handling

- All errors go to `on_error(ec, where)` where `where ∈ {"resolve","connect","read","write"}`.
- Then the client invokes `close()` → `on_close()`.
- If reconnect delay > 0, a timer schedules a reconnect attempt.

---

## 6) Request-Response Wrapping (Optional)

Applications can implement request/response patterns using **InflightTable + promise/future**.

### Example

```cpp
struct Requester {
  std::mutex m;
  std::atomic<uint32_t> seq{1};
  std::unordered_map<uint32_t, std::promise<std::string>> inflight;

  uint32_t send(TcpClient& cli, std::string_view payload) {
    uint32_t s = seq++;
    {
      std::lock_guard<std::mutex> lk(m);
      inflight.emplace(s, std::promise<std::string>{});
    }
    std::string msg = std::to_string(s) + ":" + std::string(payload) + "\n";
    cli.write((const uint8_t*)msg.data(), msg.size());
    return s;
  }

  std::future<std::string> get_future(uint32_t s) {
    std::lock_guard<std::mutex> lk(m);
    return inflight.at(s).get_future();
  }

  void fulfill(uint32_t s, std::string_view resp) {
    std::lock_guard<std::mutex> lk(m);
    if (inflight.count(s)) {
      inflight[s].set_value(std::string(resp));
      inflight.erase(s);
    }
  }
};
```

Inside `on_receive`:

```cpp
// parse frame "SEQ:payload"
uint32_t s = ...; std::string_view resp = ...;
rq.fulfill(s, resp);
```

Usage in app:

```cpp
uint32_t seq = rq.send(*cli, "PING");
auto fut = rq.get_future(seq);

if (fut.wait_for(3s) == std::future_status::ready) {
    std::string resp = fut.get(); // e.g. "PONG"
}
```

---

## 7) FAQ

- **Do I call read()?** → No, data always arrives via `on_receive()`.
- **How are frames separated?** → App must parse framing (delimiter, length-prefix, etc.).
- **Is it thread-safe?** → Inside the client, yes (strand). For shared data in your app, use your own sync.
- **Reconnect policy?** → Simple fixed delay. You can extend to exponential backoff.

---

## 8) Checklist

- [ ] Simple API: `open/close/write/reconnect_delay`
- [ ] Async callbacks: `on_open/on_close/on_receive/on_write/on_error`
- [ ] Independent of upper futures/wait (no freezing, runs in background thread)
- [ ] Request-response support via promise/future wrapper
- [ ] Race-free I/O (strand)
- [ ] Reconnect support (timer-based)

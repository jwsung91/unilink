# 📘 API Guide

This section explains how an upper-layer application can use the **TcpClient** transport.  
TcpClient is a **low-level async communication API**. Upper layers should handle parsing, state machines, and request/response logic on top.

---

## 0) Components

- **`TcpClient`** → Manages async TCP I/O (connect, send, receive, reconnect).  
- **`ICommHandler`** → Interface for receiving events (`on_open`, `on_close`, `on_receive`, `on_write`, `on_error`).

---

## 1) Initialization & Lifecycle

```cpp
boost::asio::io_context io;     
MyHandler handler;              
auto cli = std::make_shared<TcpClient>(io, handler);

// Run networking in a dedicated thread (so futures/wait won't freeze I/O)
std::thread net([&]{ io.run(); });

// Connect
cli->reconnect_delay(2s);       
cli->open("127.0.0.1", "9000");

// ... Application logic (can block on futures safely)

// Shutdown
cli->reconnect_delay(0ms);
cli->close();
io.stop();
net.join();
````

**Rule**: Run `io.run()` in a **separate thread**. This ensures that even if the main thread blocks on `future.wait()`, communication continues independently.

---

## 2) Connection/Disconnection/Reconnection

```cpp
cli->open(host, port);     // Start async connect
cli->close();              // Safe shutdown
cli->reconnect_delay(2s);  // Reconnect delay (0 disables)
```

- Results of `open()` are notified via `on_open()` or `on_error("connect")`.
- On error, the client closes, calls `on_close()`, and retries if reconnect delay > 0.

---

## 3) Send & Receive

### Send

```cpp
cli->write(reinterpret_cast<const uint8_t*>("hello\n"), 6);
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

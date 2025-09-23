# unilink

`unilink` is a C++ library that provides a unified, low-level asynchronous communication interface for TCP (client/server) and Serial ports. It simplifies network and serial programming by abstracting transport-specific details behind a consistent `IChannel` API.

This repository contains the core library and a set of examples to demonstrate its usage.

---

## Features

*   **Unified Interface**: A single `IChannel` interface for TCP (Client/Server) and Serial communication.
*   **Asynchronous Operations**: Callback-based, non-blocking I/O for high performance.
*   **Automatic Reconnection**: Built-in, configurable reconnection logic for clients and serial ports.
*   **Thread-Safe**: Managed I/O thread and thread-safe write operations.
*   **Simple Lifecycle**: Easy-to-use `start()` and `stop()` methods for managing the connection lifecycle.

---

## Project Structure

```
.
├── unilink/         # Core library source and headers
├── examples/        # Example applications (see examples/README.md)
├── docs/            # Detailed API documentation
├── CMakeLists.txt   # Top-level build script
└── README.md        # This file
```

---

## Requirements

```bash
# For the core library
sudo apt update && sudo apt install -y \
  build-essential cmake libboost-dev libboost-system-dev
```

---

## 빌드

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
cmake --build build -j
```

> `serial_chat`, `serial_echo` 예제를 쓰려면 라이브러리에 `src/transport/serial_channel.cc`가 포함되어 있어야 합니다.
> (이미 추가해두었다면 별도 작업 필요 없음)

---

## 타임스탬프 & 로그 매크로 (공통)

`include/common/common.hpp`에 아래 유틸이 **한 번**만 있으면 됩니다.

```cpp
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

inline std::string ts_now() {
  using namespace std::chrono;
  const auto now = system_clock::now();
  const auto tt  = system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif
  const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
  std::ostringstream oss;
  oss << std::put_time(&tm, "%F %T") << '.' << std::setw(3) << std::setfill('0') << ms.count();
  return oss.str(); // e.g., 2025-09-15 13:07:42.123
}
```

예제에서 쓰는 간단 매크로(선택):

```cpp
#include <sstream>
#define LOG_TX(TAG, X) do { std::ostringstream __oss; __oss << X; \
  std::cout << ts_now() << " " << TAG << " [TX] " << __oss.str() << std::endl; } while(0)
#define LOG_RX(TAG, X) do { std::ostringstream __oss; __oss << X; \
  std::cout << ts_now() << " " << TAG << " [RX] " << __oss.str() << std::endl; } while(0)
```

줄 단위 파싱에 사용하는 유틸:

```cpp
static void feed_lines(std::string& acc, const uint8_t* p, size_t n,
                       const std::function<void(std::string)>& on_line) {
  acc.append(reinterpret_cast<const char*>(p), n);
  size_t pos;
  while ((pos = acc.find('\n')) != std::string::npos) {
    std::string line = acc.substr(0, pos);
    if (!line.empty() && line.back() == '\r') line.pop_back(); // CRLF 처리
    on_line(line);
    acc.erase(0, pos + 1);
  }
}
```

---

## 예제 1) TCP 채팅

### 서버 실행

```bash
./build/tcp_chat_server 9000
```

### 클라이언트 실행 (새 터미널)

```bash
./build/tcp_chat_client 127.0.0.1 9000
```

### 사용법

* 양쪽 콘솔에서 한 줄 입력(Enter) → 상대 쪽 콘솔에 `[RX]` 로 표시
* 연결 상태는 `[server] state=...`, `[client] state=...` 로 출력

### 샘플 로그

```
2025-09-15 11:32:00.101 [server] state=Listening
2025-09-15 11:32:02.345 [server] state=Connected
2025-09-15 11:32:05.789 [server] [RX] hi from client
2025-09-15 11:32:08.222 [server] [TX] hello client
```

> **참고:** TCP는 스트림이므로 여러 메시지가 묶여 오거나 쪼개져 올 수 있습니다. 예제는 `\n` 기준으로 라인을 분리합니다.

---

## 예제 2) Serial 채팅 (가상 TTY 2개로 서로 채팅)

### 1) socat으로 가상 포트 2개를 만들고 **프로세스를 유지**

```bash
socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB
# 위 프로세스는 종료하지 말고 계속 둡니다 (브릿지 역할)
```

### 2) 채팅 프로그램 두 개 실행

```bash
./build/chat_serial /tmp/ttyA   # 터미널 2
./build/chat_serial /tmp/ttyB   # 터미널 3
```

### 3) 사용법

* 양쪽 콘솔에서 한 줄 입력(Enter) → 상대 쪽 콘솔에 `[RX]` 로 표시
* 연결 상태는 `[serial] state=Connected` 로 표시

### 샘플 로그

```
2025-09-15 11:40:10.492 [serial] state=Connecting
2025-09-15 11:40:10.492 [serial] state=Connected
2025-09-15 11:40:10.492 [serial] opened /tmp/ttyA @115200
2025-09-15 11:40:15.100 [serial] [TX] hello
2025-09-15 11:40:15.101 [serial] [RX] hi there
```

> **단독 테스트**(상대 없이 RX 확인)를 원하면 한쪽에 에코 프로그램을 띄우세요:
>
> ```bash
> ./build/chat_serial /tmp/ttyA
> ./build/chat_serial /tmp/ttyB
> ```



## 재연결 정책(요약)

* **TCP 클라이언트**: 연결 오류 시 **고정 간격**(예: 1000ms)으로 재시도하도록 코드에 반영
* **Serial**: `SerialConfig` 에서 `retry_interval_ms` (예: 2000ms)로 재오픈 간격을 설정

  * `reopen_on_error = true` 이면 자동 재오픈 시도
  * `/dev/serial/by-id/...` 사용을 권장(디바이스 경로 변경 문제 방지)

---

## 트러블슈팅

* **Serial RX가 안 보임**

  * socat 프로세스가 살아있는지 확인(브릿지를 **계속 유지**해야 합니다).
  * 고정 경로(`/tmp/ttyA`, `/tmp/ttyB`)를 사용해 경로 혼동을 피하세요.
  * 줄 단위 로그라면 개행이 필요합니다. 개행 없이 데이터가 들어오는지 확인하려면 일시적으로:

    ```cpp
    // on_bytes 안에 추가
    std::cout << ts_now() << " [serial] [RX/chunk] " << n << "B" << std::endl;
    ```

* **권한 문제**

  * `dialout` 그룹 추가 후 **로그아웃/로그인**.
* **장치 경로 변경**

  * `/dev/ttyUSB0 → USB1` 같은 변경이 잦다면 `/dev/serial/by-id/...`를 사용.

## 종료

* 예제는 모두 **Ctrl+C**로 종료.
* 서버는 클라이언트 연결이 끊겨도 **계속 Listening**(다음 연결 수락).

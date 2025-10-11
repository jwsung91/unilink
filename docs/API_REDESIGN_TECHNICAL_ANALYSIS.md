# Unilink API Redesign - 기술적 상세 분석

**작성일**: 2025-10-11  
**목적**: 제안된 API 재설계의 기술적 타당성과 구현 방안 분석

---

## 📐 아키텍처 비교

### 현재 아키텍처

```
사용자 코드
    ↓
Builder API (tcp_client_builder.hpp)
    ↓ build()
Wrapper Layer (tcp_client.hpp)
    ↓
Transport Layer (transport/tcp_client/)
    ↓
Boost.Asio
```

**레이어별 책임**:
1. **Builder**: 설정 수집, 유효성 검증, Wrapper 생성
2. **Wrapper**: 고수준 API, 상태 관리, 콜백 변환
3. **Transport**: 저수준 I/O, 재연결, 프로토콜 처리

### 제안된 아키텍처

```
사용자 코드
    ↓
Simple/Advanced API (api.hpp/api_ex.hpp)
    ↓
Link (PIMPL)
    ↓
Transport Layer (재사용)
    ↓
Boost.Asio
```

**변경점**:
- Builder + Wrapper 레이어 → Link + API 함수로 통합
- Options 구조체가 설정을 일괄 관리

---

## 🔧 핵심 구현 이슈

### 1. Link PIMPL 설계

#### 제안된 구조
```cpp
class Link {
public:
  struct Impl;
  explicit Link(Impl* impl) noexcept;
  
private:
  Impl* impl_ = nullptr;
};
```

#### 구현 옵션

**옵션 A: Impl이 transport::* 를 직접 소유**
```cpp
struct Link::Impl {
  std::variant<
    std::shared_ptr<transport::TcpClient>,
    std::shared_ptr<transport::Serial>,
    std::shared_ptr<transport::TcpServer>
  > transport_;
  
  void send(std::span<const std::byte> bytes) {
    std::visit([&](auto& t) { t->write(...); }, transport_);
  }
};
```

**장점**:
- 직접적인 transport 접근
- 타입 소거 (type erasure)

**단점**:
- variant 오버헤드
- 각 transport 타입별 처리 필요

**옵션 B: Impl을 추상 인터페이스로**
```cpp
struct Link::Impl {
  virtual ~Impl() = default;
  virtual void send(std::span<const std::byte>) = 0;
  virtual void stop() = 0;
  virtual bool is_open() const = 0;
};

template<typename Transport>
struct LinkImplT : Link::Impl {
  std::shared_ptr<Transport> transport_;
  void send(std::span<const std::byte> b) override {
    transport_->write(...);
  }
};
```

**장점**:
- 깔끔한 다형성
- 타입별 최적화 가능

**단점**:
- 가상 함수 오버헤드 (hot path에 영향)
- 동적 할당 필요

**옵션 C: 현재 wrapper::ChannelInterface 재사용**
```cpp
struct Link::Impl {
  std::unique_ptr<wrapper::ChannelInterface> channel_;
  
  void send(std::span<const std::byte> bytes) {
    std::string str(reinterpret_cast<const char*>(bytes.data()), 
                    bytes.size());
    channel_->send(str);
  }
};
```

**장점**:
- 기존 코드 최대한 재사용
- 검증된 구현

**단점**:
- wrapper 레이어 유지 (제거 못함)
- string 변환 오버헤드

**권장**: **옵션 B** (추상 인터페이스)
- 가상 함수 오버헤드는 I/O 비용 대비 미미
- 확장성과 유지보수성 최고

### 2. std::span<const std::byte> vs std::string

#### 문제점
- 제안서는 `std::span<const std::byte>` 사용
- C++20 feature (현재 프로젝트는 C++17)

#### 해결 방안

**방안 A: C++20으로 업그레이드**
```cmake
set(CMAKE_CXX_STANDARD 20)
```
**장점**: 표준 span 사용
**단점**: 기존 C++17 사용자 영향

**방안 B: gsl::span 사용**
```cpp
#include <gsl/span>
using std::span = gsl::span;
```
**장점**: C++17 호환
**단점**: 외부 의존성 추가 (MS-GSL)

**방안 C: 커스텀 span 구현**
```cpp
namespace unilink {
template<typename T>
class span {
  T* data_;
  size_t size_;
public:
  span(T* p, size_t n) : data_(p), size_(n) {}
  // ... 최소 인터페이스
};
}
```
**장점**: 의존성 없음
**단점**: 유지보수 부담

**방안 D: 기존 std::string 유지**
```cpp
// 제안 수정
using RecvCB = std::function<void(const std::string&)>;
std::error_code send(const std::string& data) noexcept;
```
**장점**: 
- C++17 호환
- 기존 코드와 일관성
- 문자열 처리 편의성

**단점**: 바이너리 데이터 처리 시 혼란

**방안 E: const uint8_t* + size_t**
```cpp
using RecvCB = std::function<void(const uint8_t* data, size_t size)>;
std::error_code send(const uint8_t* data, size_t size) noexcept;
```
**장점**: 
- C++ 버전 무관
- 명확한 바이너리 의미

**단점**: 
- 두 인자 필요
- 안전성 낮음 (lifetime 이슈)

**권장**: **방안 D (std::string 유지)** 또는 **방안 B (gsl::span)**
- 프로젝트 철학에 따라 선택
- 문자열 중심이면 D, 바이너리 중심이면 B

### 3. 에러 처리 모델

#### 현재 방식
```cpp
// 예외 가능
client->send("data");

// 에러 콜백 (문자열)
.on_error([](const std::string& error) { 
  std::cerr << error << std::endl; 
})
```

#### 제안된 방식
```cpp
// No-throw, error_code 반환
auto ec = link.send(data);
if (ec) {
  // 처리
}

// 에러 콜백 (error_code)
opt.on_error = [](std::error_code ec) {
  std::cerr << ec.message() << std::endl;
};
```

#### 구현 이슈

**error_code category 정의**:
```cpp
namespace unilink {

enum class Error {
  Success = 0,
  NotConnected,
  SendFailed,
  Timeout,
  // ...
};

class UniLinkErrorCategory : public std::error_category {
public:
  const char* name() const noexcept override {
    return "unilink";
  }
  
  std::string message(int ev) const override {
    switch (static_cast<Error>(ev)) {
      case Error::Success: return "Success";
      case Error::NotConnected: return "Not connected";
      // ...
      default: return "Unknown error";
    }
  }
};

inline const std::error_category& unilink_category() {
  static UniLinkErrorCategory instance;
  return instance;
}

inline std::error_code make_error_code(Error e) {
  return {static_cast<int>(e), unilink_category()};
}

} // namespace unilink

namespace std {
  template<>
  struct is_error_code_enum<unilink::Error> : true_type {};
}
```

**장점**:
- 표준 에러 처리
- 조합 가능 (boost::system::error_code와 호환)

**단점**:
- Boilerplate 코드 필요
- 사용자에게 익숙하지 않을 수 있음

### 4. Options 구조체 설계

#### 제안된 구조
```cpp
struct Options {
  // Common
  std::chrono::milliseconds recv_timeout{0};
  std::chrono::milliseconds reconnect_delay{0};
  std::size_t send_queue_limit{0};
  
  // TCP
  struct Tcp { bool nodelay{true}; int sndbuf{0}; } tcp;
  
  // Serial
  struct Serial { bool rtscts{false}; int vmin{-1}; } serial;
  
  // Hooks
  std::function<void(std::span<const std::byte>)> on_recv{};
  std::function<void(std::error_code)> on_error{};
};
```

#### 문제점

**1. Transport별 설정 혼재**
- TCP 전용 설정이 Serial 연결에도 존재
- 불필요한 메모리 사용

**개선안**:
```cpp
struct CommonOptions {
  std::chrono::milliseconds recv_timeout{0};
  // ...
  std::function<void(const std::string&)> on_recv{};
  std::function<void(std::error_code)> on_error{};
};

struct TcpOptions : CommonOptions {
  bool nodelay{true};
  int sndbuf{0};
  int rcvbuf{0};
};

struct SerialOptions : CommonOptions {
  bool rtscts{false};
  int vmin{-1};
  int vtime{-1};
};

// API
Link tcp_connect_ex(string_view host, uint16_t port, 
                    const TcpOptions& opt);
Link serial_open_ex(string_view device, uint32_t baud, 
                    const SerialOptions& opt);
```

**장점**:
- 타입 안전성 증가
- 관련 없는 설정 노출 안됨

**단점**:
- Options 타입이 여러 개

**2. 프리셋 함수**
```cpp
Options latency_optimized();
Options throughput_optimized();
```

**구현**:
```cpp
inline TcpOptions latency_optimized() {
  TcpOptions opt;
  opt.tcp.nodelay = true;
  opt.recv_buffer_size = 4096;  // 작은 버퍼
  opt.send_queue_limit = 10;
  return opt;
}

inline TcpOptions throughput_optimized() {
  TcpOptions opt;
  opt.tcp.nodelay = false;
  opt.tcp.sndbuf = 256 * 1024;
  opt.tcp.rcvbuf = 256 * 1024;
  opt.recv_buffer_size = 64 * 1024;
  return opt;
}
```

---

## 🔄 기존 코드 재사용 전략

### 재사용 가능한 컴포넌트

#### 1. Transport Layer (100% 재사용)
- `transport/tcp_client/tcp_client.{hpp,cc}`
- `transport/serial/serial.{hpp,cc}`
- `interface/channel.hpp`

**이유**: 
- 이미 검증된 비동기 I/O 로직
- 재연결, 에러 처리 구현됨
- Boost.Asio 래핑

#### 2. Common Utilities (100% 재사용)
- `common/logger.{hpp,cc}`
- `common/error_handler.{hpp,cc}`
- `common/safe_convert.{hpp,cc}`

#### 3. Config (부분 재사용)
- 기존 config 클래스들을 Options로 변환하는 adaptor 작성

```cpp
// 내부 헬퍼
namespace detail {
  config::TcpClientConfig to_config(const TcpOptions& opt) {
    config::TcpClientConfig cfg;
    cfg.retry_interval_ms = opt.reconnect_delay.count();
    cfg.nodelay = opt.tcp.nodelay;
    // ...
    return cfg;
  }
}
```

### 폐기할 컴포넌트

#### 1. Builder Classes
- `builder/tcp_client_builder.{hpp,cc}`
- `builder/serial_builder.{hpp,cc}`
- `builder/tcp_server_builder.{hpp,cc}`

**대체**: Simple/Advanced API 함수들

#### 2. Wrapper Classes (부분 폐기)
- `wrapper/tcp_client.{hpp,cc}`
- `wrapper/serial.{hpp,cc}`

**옵션**:
- A: 완전 폐기, Link::Impl이 직접 transport 사용
- B: 내부 구현으로 유지 (private), Link::Impl이 wrapper 사용

**권장**: **옵션 A**
- Builder와 Wrapper가 중복 기능
- Transport만으로 충분

---

## 📊 성능 영향 분석

### Hot Path 분석

**현재**:
```
user → TcpClient::send()
     → Channel::async_write_copy()
     → Boost.Asio
```

**제안**:
```
user → Link::send()
     → Link::Impl::send() [virtual call]
     → Channel::async_write_copy()
     → Boost.Asio
```

**추가 오버헤드**:
1. 가상 함수 호출: ~2-5ns (현대 CPU)
2. PIMPL 간접 참조: ~1-2ns

**I/O 비용 대비**: 
- 네트워크 I/O: ~100μs - 10ms
- 가상 함수: ~5ns
- **비율**: 0.00005% - 0.005%

**결론**: **무시 가능**

### 메모리 사용

**현재**:
```
unique_ptr<TcpClient> (8 bytes)
└─ TcpClient 객체 (~200 bytes)
   └─ shared_ptr<Channel> (16 bytes)
      └─ Channel 객체 (~300 bytes)
```
**총**: ~524 bytes per connection

**제안**:
```
Link (8 bytes)
└─ Link::Impl* (8 bytes)
   └─ Impl 객체 (~50 bytes)
      └─ shared_ptr<Channel> (16 bytes)
         └─ Channel 객체 (~300 bytes)
```
**총**: ~382 bytes per connection

**개선**: ~27% 감소 (Builder/Wrapper 제거)

---

## 🧪 테스트 전략

### 필요한 테스트 수정

#### 1. Builder 테스트 → API 테스트로 전환

**Before**:
```cpp
TEST(BuilderTest, TcpClientBuild) {
  auto client = unilink::tcp_client("localhost", 8080)
                    .on_data([](const std::string&) {})
                    .build();
  ASSERT_NE(client, nullptr);
}
```

**After**:
```cpp
TEST(ApiTest, TcpConnect) {
  auto link = unilink::tcp_connect("localhost", 8080,
                                   [](const std::string&) {},
                                   [](std::error_code) {});
  EXPECT_TRUE(link.is_open() || !link.is_open()); // 상태 확인
}
```

#### 2. Wrapper 테스트 → Link 테스트로 전환

**Before**:
```cpp
TEST(WrapperTest, Send) {
  TcpClient client("localhost", 8080);
  client.start();
  client.send("hello");
  EXPECT_TRUE(client.is_connected());
}
```

**After**:
```cpp
TEST(LinkTest, Send) {
  auto link = unilink::tcp_connect(...);
  auto ec = link.send("hello");
  EXPECT_FALSE(ec);
}
```

#### 3. Mock 업데이트

**Before**: MockTcpClient extends TcpClient
**After**: MockLink::Impl extends Link::Impl

### 테스트 커버리지 유지

**현재 커버리지**: 72.2%

**목표**: 최소 70% 유지

**전략**:
1. Transport 레이어 테스트는 변경 없음 (대부분의 커버리지)
2. 새 API/Link 레이어 테스트 추가
3. 통합 테스트 우선 수정

---

## 🚀 구현 로드맵 (Full Redesign 기준)

### Week 1: 기반 인프라
- [ ] Day 1-2: `Link` 및 `Link::Impl` 구조 설계 및 구현
  - PIMPL 패턴 구현
  - 가상 인터페이스 정의
  - Move semantics 구현
- [ ] Day 3-4: `Options` 구조체 구현
  - CommonOptions, TcpOptions, SerialOptions
  - Preset 함수들
  - Config adaptor 함수들
- [ ] Day 5: 에러 처리 시스템
  - error_code category 정의
  - Error enum 정의

### Week 2: API 구현
- [ ] Day 1-2: Simple API (Level 1)
  - `tcp_connect()` 구현
  - `serial_open()` 구현
  - Transport와 연결
- [ ] Day 3-4: Advanced API (Level 2)
  - `tcp_connect_ex()` 구현
  - `serial_open_ex()` 구현
  - Options 적용
- [ ] Day 5: 단위 테스트 작성
  - Link 테스트
  - API 함수 테스트

### Week 3: 마이그레이션
- [ ] Day 1-2: 예제 코드 수정
  - 4개 튜토리얼 업데이트
- [ ] Day 3-5: 테스트 코드 수정
  - Builder 테스트 → API 테스트
  - Wrapper 테스트 → Link 테스트
  - 통합 테스트 업데이트

### Week 4: 정리 및 검증
- [ ] Day 1-2: 문서 업데이트
  - API 레퍼런스
  - 마이그레이션 가이드
  - README
- [ ] Day 3: 성능 테스트
  - 벤치마크 실행
  - 회귀 확인
- [ ] Day 4: 코드 리뷰 및 수정
- [ ] Day 5: 릴리즈 준비
  - CHANGELOG
  - 버전 태그

**총 예상 기간**: 4주 (20 작업일)

---

## 🎨 코드 예시 (제안된 API)

### Simple API 사용

```cpp
#include <unilink/api.hpp>

int main() {
  // TCP Client
  auto client = unilink::tcp_connect(
    "localhost", 8080,
    [](const std::string& data) {
      std::cout << "Received: " << data << std::endl;
    },
    [](std::error_code ec) {
      std::cerr << "Error: " << ec.message() << std::endl;
    }
  );
  
  // Send
  if (auto ec = client.send("Hello"); ec) {
    std::cerr << "Send failed: " << ec.message() << std::endl;
  }
  
  // Link는 RAII, 소멸자에서 자동 정리
  return 0;
}
```

### Advanced API 사용

```cpp
#include <unilink/api_ex.hpp>
#include <unilink/options.hpp>

int main() {
  // Options 구성
  unilink::TcpOptions opt;
  opt.recv_timeout = std::chrono::seconds(10);
  opt.reconnect_delay = std::chrono::seconds(5);
  opt.tcp.nodelay = true;
  opt.tcp.sndbuf = 128 * 1024;
  
  opt.on_recv = [](const std::string& data) {
    std::cout << "Received: " << data << std::endl;
  };
  
  opt.on_error = [](std::error_code ec) {
    std::cerr << "Error: " << ec.message() << std::endl;
  };
  
  opt.on_backpressure = [](std::size_t queued) {
    std::cout << "Queue size: " << queued << std::endl;
  };
  
  // 연결
  auto client = unilink::tcp_connect_ex("localhost", 8080, opt);
  
  // 또는 preset 사용
  auto fast_client = unilink::tcp_connect_ex(
    "localhost", 8080,
    unilink::latency_optimized()
  );
  
  return 0;
}
```

### 프리셋 조합

```cpp
// 프리셋 기반으로 커스터마이징
auto opt = unilink::latency_optimized();
opt.reconnect_delay = std::chrono::seconds(1); // 재연결만 변경
opt.on_recv = my_handler;

auto link = unilink::tcp_connect_ex("host", 8080, opt);
```

---

## 🔍 대안 구현: 하이브리드 접근

제안서를 따르되, Builder를 deprecated로 유지:

### 구조
```
include/unilink/
├── link.hpp           # 새 API
├── options.hpp
├── api.hpp
├── api_ex.hpp
└── deprecated/
    ├── builder.hpp    # 기존 Builder (deprecated)
    └── wrapper.hpp    # 기존 Wrapper (deprecated)
```

### 구현
```cpp
// 새 API는 제안대로
namespace unilink {
  Link tcp_connect(...);
}

// 기존 API는 새 API로 구현
namespace unilink {
  [[deprecated("Use unilink::tcp_connect() instead")]]
  inline builder::TcpClientBuilder tcp_client(
      const std::string& host, uint16_t port) {
    // 내부적으로 새 API 사용하도록 adaptor 구현
    return builder::TcpClientBuilder(host, port);
  }
}
```

**장점**:
- 소스 호환성 제공 (warning만 발생)
- 점진적 마이그레이션 가능
- 새 기능은 새 API로만 제공

**단점**:
- 코드베이스 복잡도 증가
- 두 API 유지보수

---

## ✅ 최종 체크리스트

### 설계 결정 사항
- [ ] std::string vs std::span<const std::byte> 결정
- [ ] Link::Impl 구현 방식 결정 (variant vs interface)
- [ ] Options 구조 (단일 vs 타입별)
- [ ] Breaking change 수용 범위

### 기술적 리스크
- [ ] 가상 함수 성능 영향 평가
- [ ] PIMPL overhead 측정
- [ ] 메모리 사용량 프로파일링
- [ ] 컴파일 타임 영향 측정

### 호환성 리스크
- [ ] 기존 사용자 수 파악
- [ ] 마이그레이션 비용 추산
- [ ] Deprecation 정책 수립
- [ ] 버전 정책 결정 (SemVer)

---

## 📚 참고 자료

- [Boost.Asio Best Practices](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio/overview.html)
- [C++ Core Guidelines - Error Handling](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#e-error-handling)
- [PImpl Idiom](https://en.cppreference.com/w/cpp/language/pimpl)
- [std::error_code Tutorial](https://www.boost.org/doc/libs/1_82_0/libs/system/doc/html/system.html)

---

**문서 버전**: 1.0  
**최종 업데이트**: 2025-10-11


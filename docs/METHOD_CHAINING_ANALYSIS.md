# 메서드 체이닝 API 분석 및 Simple/Advanced 구분

**작성일**: 2025-10-11  
**목적**: 현재 Builder API의 메서드를 Simple/Advanced로 구분하여 단순화 방안 제시

---

## 📊 현재 메서드 체이닝 API 분석

### 1. TcpClientBuilder

#### 전체 메서드 목록 (9개)

| # | 메서드 | 설명 | 사용 빈도 | 카테고리 |
|---|--------|------|----------|----------|
| 1 | `on_data(handler)` | 데이터 수신 콜백 | ⭐⭐⭐ 필수 | Core |
| 2 | `on_error(handler)` | 에러 처리 콜백 | ⭐⭐⭐ 필수 | Core |
| 3 | `on_connect(handler)` | 연결 성공 콜백 | ⭐⭐ 자주 | Core |
| 4 | `on_disconnect(handler)` | 연결 해제 콜백 | ⭐⭐ 자주 | Core |
| 5 | `auto_start(bool)` | 자동 시작 | ⭐⭐ 자주 | Convenience |
| 6 | `auto_manage(bool)` | 자동 관리 | ⭐ 가끔 | Convenience |
| 7 | `retry_interval(ms)` | 재연결 간격 | ⭐ 가끔 | Advanced |
| 8 | `use_independent_context(bool)` | 독립 Context (테스트용) | △ 거의안씀 | Testing |
| 9 | `build()` | 빌드 실행 | ⭐⭐⭐ 필수 | Core |

**템플릿 오버로드 제외 시**: 9개 메서드

---

### 2. SerialBuilder

#### 전체 메서드 목록 (9개)

| # | 메서드 | 설명 | 사용 빈도 | 카테고리 |
|---|--------|------|----------|----------|
| 1 | `on_data(handler)` | 데이터 수신 콜백 | ⭐⭐⭐ 필수 | Core |
| 2 | `on_error(handler)` | 에러 처리 콜백 | ⭐⭐⭐ 필수 | Core |
| 3 | `on_connect(handler)` | 연결 성공 콜백 | ⭐⭐ 자주 | Core |
| 4 | `on_disconnect(handler)` | 연결 해제 콜백 | ⭐⭐ 자주 | Core |
| 5 | `auto_start(bool)` | 자동 시작 | ⭐⭐ 자주 | Convenience |
| 6 | `auto_manage(bool)` | 자동 관리 | ⭐ 가끔 | Convenience |
| 7 | `retry_interval(ms)` | 재연결 간격 | ⭐ 가끔 | Advanced |
| 8 | `use_independent_context(bool)` | 독립 Context (테스트용) | △ 거의안씀 | Testing |
| 9 | `build()` | 빌드 실행 | ⭐⭐⭐ 필수 | Core |

**TcpClient와 동일**: 9개 메서드

---

### 3. TcpServerBuilder

#### 전체 메서드 목록 (16개)

| # | 메서드 | 설명 | 사용 빈도 | 카테고리 |
|---|--------|------|----------|----------|
| 1 | `on_data(handler)` | 데이터 수신 (단순) | ⭐⭐⭐ 필수 | Core |
| 2 | `on_data(client_handler)` | 데이터 수신 (클라이언트 ID 포함) | ⭐⭐ 자주 | Core |
| 3 | `on_error(handler)` | 에러 처리 | ⭐⭐⭐ 필수 | Core |
| 4 | `on_connect(handler)` | 연결 성공 (단순) | ⭐⭐ 자주 | Core |
| 5 | `on_connect(client_handler)` | 연결 성공 (클라이언트 정보) | ⭐⭐ 자주 | Core |
| 6 | `on_disconnect(handler)` | 연결 해제 (단순) | ⭐⭐ 자주 | Core |
| 7 | `on_disconnect(client_handler)` | 연결 해제 (클라이언트 ID) | ⭐⭐ 자주 | Core |
| 8 | `auto_start(bool)` | 자동 시작 | ⭐⭐ 자주 | Convenience |
| 9 | `auto_manage(bool)` | 자동 관리 | ⭐ 가끔 | Convenience |
| 10 | `single_client()` | 단일 클라이언트 모드 | ⭐⭐ 자주 | Mode |
| 11 | `multi_client(max)` | 다중 클라이언트 모드 | ⭐⭐ 자주 | Mode |
| 12 | `unlimited_clients()` | 무제한 클라이언트 | ⭐ 가끔 | Mode |
| 13 | `on_multi_connect(handler)` | 다중 연결 콜백 | ⭐ 가끔 | Multi-Client |
| 14 | `on_multi_data(handler)` | 다중 데이터 콜백 | ⭐ 가끔 | Multi-Client |
| 15 | `on_multi_disconnect(handler)` | 다중 해제 콜백 | ⭐ 가끔 | Multi-Client |
| 16 | `max_clients(size)` | 최대 클라이언트 수 | ⭐ 가끔 | Advanced |
| 17 | `enable_port_retry(...)` | 포트 재시도 | △ 거의안씀 | Advanced |
| 18 | `use_independent_context(bool)` | 독립 Context | △ 거의안씀 | Testing |
| 19 | `build()` | 빌드 실행 | ⭐⭐⭐ 필수 | Core |

**실제**: 19개 메서드 (오버로드 포함)

---

## 🎯 Simple API vs Advanced API 제안

### 전략: 메서드 체이닝 유지하되 개수 차등화

```cpp
// Simple API - 최소 메서드만
auto client = unilink::simple::tcp_client("host", 8080)
                  .on_data([](auto& data) { })
                  .build();

// Advanced API - 전체 메서드
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](auto& data) { })
                  .on_connect([](){ })
                  .on_disconnect([](){ })
                  .on_error([](auto& err) { })
                  .retry_interval(5000)
                  .auto_start(true)
                  .build();
```

---

## 📉 Simple API 제안

### TcpClient Simple (9개 → 4개)

| 메서드 | 필수? | 설명 |
|--------|-------|------|
| `on_data(handler)` | ✅ | 데이터 수신 콜백 |
| `on_error(handler)` | ✅ | 에러 처리 콜백 |
| `auto_start(bool = true)` | 기본값 true | 자동 시작 (기본 활성화) |
| `build()` | ✅ | 빌드 실행 |

**제거된 메서드 (5개)**:
- ❌ `on_connect()` → Simple에서는 자동 연결
- ❌ `on_disconnect()` → Simple에서는 무시
- ❌ `auto_manage()` → 항상 true
- ❌ `retry_interval()` → 기본값(2초) 사용
- ❌ `use_independent_context()` → 테스트 전용

**사용 예시**:
```cpp
// 최소 사용 (2개 메서드)
auto client = unilink::simple::tcp_client("localhost", 8080)
                  .on_data([](const std::string& data) {
                    std::cout << data << std::endl;
                  })
                  .build();

// 에러 처리 추가 (3개 메서드)
auto client = unilink::simple::tcp_client("localhost", 8080)
                  .on_data([](auto& data) { process(data); })
                  .on_error([](auto& err) { log_error(err); })
                  .build();
```

---

### Serial Simple (9개 → 4개)

동일하게 4개 메서드만 제공

| 메서드 | 필수? | 설명 |
|--------|-------|------|
| `on_data(handler)` | ✅ | 데이터 수신 콜백 |
| `on_error(handler)` | ✅ | 에러 처리 콜백 |
| `auto_start(bool = true)` | 기본값 true | 자동 시작 |
| `build()` | ✅ | 빌드 실행 |

---

### TcpServer Simple (19개 → 6개)

| 메서드 | 필수? | 설명 |
|--------|-------|------|
| `on_data(handler)` | ✅ | 데이터 수신 (클라이언트 ID 포함) |
| `on_error(handler)` | ✅ | 에러 처리 |
| `single_client()` | 선택 | 단일 클라이언트 모드 (기본) |
| `multi_client(max)` | 선택 | 다중 클라이언트 모드 |
| `auto_start(bool = true)` | 기본값 true | 자동 시작 |
| `build()` | ✅ | 빌드 실행 |

**제거된 메서드 (13개)**:
- ❌ `on_connect()` 오버로드들
- ❌ `on_disconnect()` 오버로드들
- ❌ `on_multi_*()` 시리즈 → 기본 콜백으로 통합
- ❌ `unlimited_clients()` → `multi_client(0)` 로 대체
- ❌ `max_clients()` → `multi_client()`에 통합
- ❌ `enable_port_retry()` → 기본 활성화
- ❌ `use_independent_context()`
- ❌ `auto_manage()`

**사용 예시**:
```cpp
// Echo Server (최소 2개 메서드)
auto server = unilink::simple::tcp_server(8080)
                  .on_data([](size_t client_id, const std::string& data) {
                    std::cout << "Client " << client_id << ": " << data;
                  })
                  .build();

// Multi-client Chat Server (3개 메서드)
auto server = unilink::simple::tcp_server(8080)
                  .multi_client(10)
                  .on_data([](size_t id, const std::string& msg) {
                    broadcast_to_all(msg);
                  })
                  .build();
```

---

## 📊 요약 비교표

### 메서드 개수 비교

| Builder | 현재 (Advanced) | Simple | 감소량 |
|---------|----------------|--------|--------|
| **TcpClient** | 9개 | **4개** | -5개 (-56%) |
| **Serial** | 9개 | **4개** | -5개 (-56%) |
| **TcpServer** | 19개 | **6개** | -13개 (-68%) |

### 최소 사용 시 메서드 개수

| Builder | 현재 최소 | Simple 최소 | 차이 |
|---------|----------|------------|------|
| **TcpClient** | 3개 (on_data + auto_start + build) | **2개** (on_data + build) | -1개 |
| **Serial** | 3개 | **2개** | -1개 |
| **TcpServer** | 3개 | **2개** | -1개 |

---

## 💡 현재도 최소 사용 가능?

### ✅ 예, 이미 가능합니다!

**현재 최소 사용 예시**:
```cpp
// TcpClient 최소 (3개 메서드)
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](auto& d) { std::cout << d; })
                  .auto_start(true)
                  .build();

// Serial 최소 (3개 메서드)
auto serial = unilink::serial("/dev/ttyUSB0", 115200)
                  .on_data([](auto& d) { std::cout << d; })
                  .auto_start(true)
                  .build();

// TcpServer 최소 (3개 메서드)
auto server = unilink::tcp_server(8080)
                  .on_data([](auto& d) { std::cout << d; })
                  .auto_start(true)
                  .build();
```

**하지만**:
- ⚠️ `auto_start(true)` 명시 필요 (기본값 false)
- ⚠️ 에러 처리 없음 (on_error 선택사항)
- ⚠️ 9~19개 메서드 중 선택해야 함 (혼란 가능)

---

## 🎨 Simple API 상세 설계

### 구현 방식 1: 상속 기반

```cpp
namespace unilink {
namespace simple {

// Simple은 Advanced를 상속하되 일부 메서드만 노출
class TcpClientBuilderSimple : public builder::TcpClientBuilder {
public:
  using builder::TcpClientBuilder::TcpClientBuilder;
  
  // 노출할 메서드만 public으로 재선언
  using builder::TcpClientBuilder::on_data;
  using builder::TcpClientBuilder::on_error;
  using builder::TcpClientBuilder::build;
  
  // auto_start는 기본값 변경하여 재정의
  TcpClientBuilderSimple& auto_start(bool start = true) {
    builder::TcpClientBuilder::auto_start(start);
    return *this;
  }
  
  // 나머지 메서드는 private로 숨김
private:
  using builder::TcpClientBuilder::on_connect;
  using builder::TcpClientBuilder::on_disconnect;
  using builder::TcpClientBuilder::auto_manage;
  using builder::TcpClientBuilder::retry_interval;
  using builder::TcpClientBuilder::use_independent_context;
};

// Convenience 함수
inline TcpClientBuilderSimple tcp_client(const std::string& host, uint16_t port) {
  return TcpClientBuilderSimple(host, port);
}

} // namespace simple
} // namespace unilink
```

**장점**:
- ✅ 코드 중복 없음
- ✅ 내부 구현 재사용
- ✅ 유지보수 쉬움

**단점**:
- ⚠️ private으로 숨긴 메서드도 메모리에 존재
- ⚠️ 상속 관계 노출

---

### 구현 방식 2: Wrapper 기반

```cpp
namespace unilink {
namespace simple {

class TcpClientBuilderSimple {
public:
  TcpClientBuilderSimple(const std::string& host, uint16_t port)
      : impl_(host, port) {
    // Simple의 기본값 설정
    impl_.auto_start(true);
    impl_.auto_manage(true);
  }
  
  TcpClientBuilderSimple& on_data(std::function<void(const std::string&)> handler) {
    impl_.on_data(std::move(handler));
    return *this;
  }
  
  TcpClientBuilderSimple& on_error(std::function<void(const std::string&)> handler) {
    impl_.on_error(std::move(handler));
    return *this;
  }
  
  TcpClientBuilderSimple& auto_start(bool start = true) {
    impl_.auto_start(start);
    return *this;
  }
  
  std::unique_ptr<wrapper::TcpClient> build() {
    return impl_.build();
  }
  
private:
  builder::TcpClientBuilder impl_;
};

} // namespace simple
} // namespace unilink
```

**장점**:
- ✅ 완전한 인터페이스 제어
- ✅ Simple 전용 기본값 설정 가능
- ✅ 명확한 분리

**단점**:
- ⚠️ Forwarding 코드 중복
- ⚠️ 약간의 성능 오버헤드 (인라인으로 해결 가능)

---

### 구현 방식 3: 팩토리 함수 (가장 단순)

```cpp
namespace unilink {
namespace simple {

// Simple은 그냥 Advanced Builder를 반환하되
// 기본값을 미리 설정해서 반환
inline builder::TcpClientBuilder tcp_client(const std::string& host, uint16_t port) {
  return builder::TcpClientBuilder(host, port)
      .auto_start(true)    // Simple은 기본적으로 자동 시작
      .auto_manage(true);  // Simple은 기본적으로 자동 관리
}

} // namespace simple

// 사용
auto client = unilink::simple::tcp_client("host", 8080)
                  .on_data([](auto& d) {})  // 여전히 모든 메서드 접근 가능
                  .retry_interval(5000)      // Advanced 메서드도 사용 가능!
                  .build();
```

**장점**:
- ✅ 구현 가장 단순 (1줄)
- ✅ 코드 중복 없음
- ✅ Simple에서도 필요시 Advanced 메서드 사용 가능

**단점**:
- ⚠️ 메서드 개수 제한 안됨 (IDE에서 여전히 전부 보임)
- ⚠️ Simple/Advanced 구분이 명확하지 않음

---

## 🎯 권장 방식

### ✨ **방식 2 (Wrapper 기반) + 방식 3 (팩토리) 조합**

```cpp
namespace unilink {

// Advanced API (기존 유지)
namespace builder {
  class TcpClientBuilder { /* 전체 메서드 */ };
}

// Simple API (새로 추가)
namespace simple {
  class TcpClientBuilderSimple {
    // 4개 메서드만 노출
    TcpClientBuilderSimple& on_data(...);
    TcpClientBuilderSimple& on_error(...);
    TcpClientBuilderSimple& auto_start(bool = true);
    std::unique_ptr<...> build();
    
  private:
    builder::TcpClientBuilder impl_;  // 내부적으로 Advanced 사용
  };
  
  // Convenience
  inline TcpClientBuilderSimple tcp_client(const std::string& host, uint16_t port) {
    return TcpClientBuilderSimple(host, port);
  }
}

// 사용자는 선택
auto simple_client = unilink::simple::tcp_client("host", 8080)  // 4개 메서드만
                         .on_data([](auto& d) {})
                         .build();

auto advanced_client = unilink::tcp_client("host", 8080)        // 9개 메서드 전부
                           .on_data([](auto& d) {})
                           .on_connect([]() {})
                           .retry_interval(5000)
                           .build();
```

**이유**:
1. ✅ **명확한 구분** - Simple은 4개, Advanced는 9개
2. ✅ **IDE 친화적** - Simple 사용 시 IDE 자동완성에 4개만 표시
3. ✅ **점진적 학습** - Simple로 시작 → 필요시 Advanced로
4. ✅ **호환성** - 기존 코드 영향 없음

---

## 📋 구현 작업량

### Phase 1: Simple API 추가

| 작업 | 파일 | 예상 시간 |
|------|------|----------|
| TcpClientBuilderSimple | `unilink/simple/tcp_client_builder.hpp/cc` | 2시간 |
| SerialBuilderSimple | `unilink/simple/serial_builder.hpp/cc` | 2시간 |
| TcpServerBuilderSimple | `unilink/simple/tcp_server_builder.hpp/cc` | 3시간 |
| Convenience 함수 | `unilink/simple.hpp` | 1시간 |
| 단위 테스트 | `test/unit/simple/` | 3시간 |
| 예제 | `examples/simple/` | 2시간 |
| 문서 | `docs/guides/SIMPLE_API.md` | 2시간 |
| **총계** | - | **15시간 (2일)** |

---

## 🎨 사용 예시 비교

### 현재 API (Advanced)
```cpp
// 최소 사용
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](const std::string& data) {
                    std::cout << data << std::endl;
                  })
                  .auto_start(true)
                  .build();

// 풀 사용
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](auto& d) { process(d); })
                  .on_connect([]() { log("connected"); })
                  .on_disconnect([]() { log("disconnected"); })
                  .on_error([](auto& e) { handle_error(e); })
                  .retry_interval(5000)
                  .auto_start(true)
                  .auto_manage(true)
                  .build();
```

### 제안된 Simple API
```cpp
// Simple (최소)
auto client = unilink::simple::tcp_client("localhost", 8080)
                  .on_data([](const std::string& data) {
                    std::cout << data << std::endl;
                  })
                  .build();  // auto_start는 기본 true

// Simple (에러 처리 추가)
auto client = unilink::simple::tcp_client("localhost", 8080)
                  .on_data([](auto& d) { process(d); })
                  .on_error([](auto& e) { handle_error(e); })
                  .build();
```

### 코드 라인 수 비교
- **현재 최소**: 5줄 (tcp_client ~ build)
- **Simple 최소**: 4줄 (-1줄, -20%)
- **현재 풀**: 10줄
- **Simple**: 4~5줄 고정

---

## ✅ 결론

### 메서드 개수 요약

| API | TcpClient | Serial | TcpServer |
|-----|-----------|--------|-----------|
| **현재 (Advanced)** | 9개 | 9개 | 19개 |
| **제안 (Simple)** | **4개** | **4개** | **6개** |
| **감소량** | -5개 (-56%) | -5개 (-56%) | -13개 (-68%) |

### 최소 사용 시

| API | 현재 | Simple | 개선 |
|-----|------|--------|------|
| **메서드 호출 수** | 3개 | 2개 | -1개 (-33%) |
| **코드 라인** | 5줄 | 4줄 | -1줄 (-20%) |
| **IDE 자동완성 항목** | 9~19개 | 4~6개 | -5~13개 |

### 핵심 메시지

> **"현재도 최소 3개 메서드로 사용 가능하지만, Simple API는 선택지를 4~6개로 제한하여 학습 곡선을 낮춥니다."**

**다음 단계**: Simple API 구현 (2일 작업)

---

**Last Updated**: 2025-10-11


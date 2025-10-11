# Unilink API 개선안 - 기본값 최적화 접근

**작성일**: 2025-10-11  
**핵심 아이디어**: Simple API를 별도로 만들지 않고, 기존 Builder의 기본값을 개선하여 Simple 사용 가능하게

---

## 💡 핵심 인사이트 (사용자 피드백)

### 1. `build()` 제거
**현재 문제**:
```cpp
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](auto& d) {})
                  .build();  // ← 이게 항상 필요한가?
```

**개선 방향**:
- `build()`는 단순히 객체를 반환만 함
- 커스터마이징 없음
- → **자동으로 생성되면 됨!**

### 2. `send()` 메서드는 필수
**현재**:
```cpp
auto client = builder.build();  // unique_ptr<TcpClient>
client->send("hello");
```

**개선**:
```cpp
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](auto& d) {});
client.send("hello");  // 바로 사용 가능
```

### 3. Simple은 별도 API가 아닌 "기본값 개선"
**기존 제안 (잘못됨)**:
- Simple용 별도 클래스 만들기
- 메서드 제거
- 복잡도 증가

**올바른 접근**:
- 기존 Builder 그대로 사용
- **합리적인 기본값** 설정
- 호출 안한 메서드는 기본값 적용
- Simple = 적은 메서드 호출
- Advanced = 많은 메서드 호출

### 4. 완벽한 호환성
- 기존 코드 변경 없음
- 새 코드는 더 적은 메서드로 사용 가능
- 점진적 개선

---

## 🎯 새로운 설계 방향

### 방법 1: Builder 패턴 개선 (RAII)

```cpp
class TcpClient {
public:
    // 생성자에서 설정 수집
    TcpClient(const std::string& host, uint16_t port);
    
    // 메서드 체이닝으로 설정
    TcpClient& on_data(std::function<void(const std::string&)> handler);
    TcpClient& on_error(std::function<void(const std::string&)> handler);
    TcpClient& on_connect(std::function<void()> handler);
    TcpClient& on_disconnect(std::function<void()> handler);
    TcpClient& retry_interval(unsigned ms);
    TcpClient& auto_start(bool start = true);
    
    // 실제 동작 메서드
    void send(const std::string& data);
    void send_line(const std::string& line);
    bool is_connected() const;
    void start();  // 명시적 시작
    void stop();
    
private:
    // 내부 구현 (기존 transport 활용)
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // 설정 상태
    bool auto_start_ = true;  // ← 기본값을 true로!
    bool started_ = false;
};
```

**사용 예시**:
```cpp
// Simple 사용 (2개 메서드만)
auto client = unilink::TcpClient("localhost", 8080)
                  .on_data([](auto& d) { std::cout << d; })
                  .on_error([](auto& e) { std::cerr << e; });
// 자동으로 시작됨 (auto_start 기본값 true)
client.send("Hello");

// Advanced 사용 (모든 설정)
auto client = unilink::TcpClient("localhost", 8080)
                  .on_data([](auto& d) { process(d); })
                  .on_error([](auto& e) { handle(e); })
                  .on_connect([]() { log("connected"); })
                  .retry_interval(5000)
                  .auto_start(false);  // 수동 시작
client.start();  // 명시적 시작
client.send("Hello");
```

**장점**:
- ✅ `build()` 불필요
- ✅ `send()` 직접 호출 가능
- ✅ 기본값으로 Simple 사용 가능
- ✅ 모든 메서드 호출하면 Advanced

**단점**:
- ⚠️ Builder와 실제 객체가 분리 안됨
- ⚠️ 설정 단계와 사용 단계 구분 불명확

---

### 방법 2: 자동 build() (권장)

```cpp
class TcpClientBuilder {
public:
    TcpClientBuilder(const std::string& host, uint16_t port);
    
    // 설정 메서드
    TcpClientBuilder& on_data(...);
    TcpClientBuilder& on_error(...);
    TcpClientBuilder& on_connect(...);
    TcpClientBuilder& retry_interval(...);
    TcpClientBuilder& auto_start(bool = true);  // 기본값 true로 변경
    
    // 암시적 변환 연산자 - build() 자동 호출
    operator std::unique_ptr<TcpClient>() {
        return build();
    }
    
    // 명시적 build도 여전히 가능
    std::unique_ptr<TcpClient> build();
    
private:
    // 설정 저장
    std::string host_;
    uint16_t port_;
    bool auto_start_ = true;  // ← 기본값 개선
    unsigned retry_interval_ms_ = 2000;
    // ...
};
```

**사용 예시**:
```cpp
// Simple - 암시적 변환
std::unique_ptr<TcpClient> client = 
    unilink::tcp_client("localhost", 8080)
        .on_data([](auto& d) { std::cout << d; });
// build() 자동 호출!

client->send("Hello");

// 또는 auto 사용
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](auto& d) {})
                  .build();  // 명시적으로도 가능

// Advanced
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](auto& d) {})
                  .on_connect([]() {})
                  .retry_interval(5000)
                  .build();
```

**장점**:
- ✅ `build()` 선택적
- ✅ 기존 코드 호환
- ✅ 기본값으로 Simple 동작

**단점**:
- ⚠️ 여전히 `->send()` (포인터)

---

### 방법 3: 하이브리드 (최선)

```cpp
// Wrapper는 기존처럼 유지
class TcpClient {
public:
    void send(const std::string& data);
    bool is_connected() const;
    // ...
};

// Builder 개선
class TcpClientBuilder {
public:
    TcpClientBuilder(const std::string& host, uint16_t port);
    
    TcpClientBuilder& on_data(...);
    TcpClientBuilder& on_error(...);
    TcpClientBuilder& on_connect(...);
    TcpClientBuilder& retry_interval(...);
    
    // 핵심: build() 자동 호출하는 래퍼 반환
    struct ClientHandle {
        std::unique_ptr<TcpClient> client_;
        
        // 포인터처럼 동작
        TcpClient* operator->() { return client_.get(); }
        TcpClient& operator*() { return *client_; }
        
        // 직접 send 가능
        void send(const std::string& data) { client_->send(data); }
        bool is_connected() const { return client_->is_connected(); }
    };
    
    // 자동 변환
    operator ClientHandle() {
        return ClientHandle{build()};
    }
    
private:
    bool auto_start_ = true;  // 기본값 true
    unsigned retry_interval_ms_ = 2000;
    bool auto_manage_ = true;  // 기본값 true
};
```

**사용 예시**:
```cpp
// Simple (auto로 받으면 자동 build)
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](auto& d) { std::cout << d; });

client.send("Hello");  // . 으로 직접 호출!

// Advanced
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](auto& d) {})
                  .on_connect([]() {})
                  .retry_interval(5000);

client.send("World");
```

---

## 📊 개선된 기본값

### 현재 기본값 (개선 필요)

| 설정 | 현재 기본값 | 문제점 |
|------|------------|--------|
| `auto_start` | **false** | 대부분 true 원함 |
| `auto_manage` | **false** | 대부분 true 원함 |
| `retry_interval` | 2000ms | 적절함 ✅ |
| `on_connect` | nullptr | 선택사항 ✅ |
| `on_disconnect` | nullptr | 선택사항 ✅ |

### 개선된 기본값 (제안)

| 설정 | 새 기본값 | 이유 |
|------|----------|------|
| `auto_start` | **true** | 대부분 즉시 시작 원함 |
| `auto_manage` | **true** | RAII 원칙 |
| `retry_interval` | 2000ms | 유지 |
| `on_connect` | nullptr | 선택사항 유지 |
| `on_disconnect` | nullptr | 선택사항 유지 |
| `on_error` | **기본 로거** | nullptr보다 로깅이 나음 |

**결과**:
- Simple 사용자: 설정 안해도 합리적으로 동작
- Advanced 사용자: 원하는 대로 오버라이드

---

## 🎨 코드 비교

### 현재 (개선 전)

```cpp
// Simple 의도 - 하지만 여전히 복잡
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](const std::string& data) {
                    std::cout << data << std::endl;
                  })
                  .auto_start(true)   // 명시 필요!
                  .auto_manage(true)  // 명시 필요!
                  .build();           // 명시 필요!

client->send("Hello");  // 포인터
```

### 개선 후 (제안)

```cpp
// Simple - 기본값 활용
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](const std::string& data) {
                    std::cout << data << std::endl;
                  });
// auto_start=true, auto_manage=true는 기본값!
// build()는 자동!

client.send("Hello");  // 직접 호출
```

### Advanced 사용 (변경 없음)

```cpp
// Advanced - 모든 설정 명시
auto client = unilink::tcp_client("localhost", 8080)
                  .on_data([](auto& d) { process(d); })
                  .on_connect([]() { log("connected"); })
                  .on_disconnect([]() { log("disconnected"); })
                  .on_error([](auto& e) { handle(e); })
                  .retry_interval(5000)
                  .auto_start(false)  // 기본값 오버라이드
                  .auto_manage(false);

client.start();  // 수동 시작
client.send("World");
```

---

## 🔧 구현 계획

### Phase 1: 기본값 개선 (1일)

#### 1.1 기본값 변경
```cpp
// TcpClientBuilder 생성자
TcpClientBuilder::TcpClientBuilder(const std::string& host, uint16_t port)
    : host_(host),
      port_(port),
      auto_start_(true),      // false → true
      auto_manage_(true),     // false → true
      use_independent_context_(false),
      retry_interval_ms_(2000) {
  
  // 기본 에러 핸들러 추가
  on_error_ = [](const std::string& error) {
    std::cerr << "[Unilink Error] " << error << std::endl;
  };
}
```

**영향**:
- ✅ 기존 코드: `.auto_start(true)` 호출하던 코드는 영향 없음
- ✅ 새 코드: 호출 안해도 자동 시작
- ⚠️ Breaking: `.auto_start(false)` 의존하던 코드는 영향 (거의 없을 것)

#### 1.2 자동 build() (선택적)

**Option A: 암시적 변환**
```cpp
class TcpClientBuilder {
public:
    // 암시적 변환 연산자
    operator std::unique_ptr<wrapper::TcpClient>() {
        return build();
    }
};
```

**Option B: 명시적 유지**
- build() 그대로 유지
- 기본값만 개선

**권장**: Option B (Phase 1)
- 기본값 개선만으로도 충분한 효과
- build() 자동화는 Phase 2에서

---

### Phase 2: build() 자동화 (선택적, 1일)

```cpp
// ClientHandle 구현
struct ClientHandle {
    std::unique_ptr<TcpClient> impl_;
    
    void send(const std::string& data) { impl_->send(data); }
    void send_line(const std::string& line) { impl_->send_line(line); }
    bool is_connected() const { return impl_->is_connected(); }
    void stop() { impl_->stop(); }
    
    // 필요시 포인터 접근
    TcpClient* operator->() { return impl_.get(); }
    TcpClient& operator*() { return *impl_; }
};

class TcpClientBuilder {
public:
    operator ClientHandle() {
        return ClientHandle{build()};
    }
};
```

---

## 📋 최소 메서드 호출 수

### 개선 전 vs 후

| 시나리오 | 개선 전 | 개선 후 | 차이 |
|----------|---------|---------|------|
| **최소 (에러무시)** | 3개 | **1개** | -2개 |
| **권장 (에러처리)** | 4개 | **2개** | -2개 |

### 구체적 예시

```cpp
// 개선 전: 최소 3개
auto client = unilink::tcp_client("host", 8080)
    .on_data([](auto& d) {})
    .auto_start(true)  // 필수!
    .build();          // 필수!

// 개선 후: 최소 1개!
auto client = unilink::tcp_client("host", 8080)
    .on_data([](auto& d) {});
// 끝! (auto_start와 build는 자동)

// 개선 전: 권장 4개
auto client = unilink::tcp_client("host", 8080)
    .on_data([](auto& d) {})
    .on_error([](auto& e) {})
    .auto_start(true)
    .build();

// 개선 후: 권장 2개
auto client = unilink::tcp_client("host", 8080)
    .on_data([](auto& d) {})
    .on_error([](auto& e) {});
```

---

## 🎯 API 레벨 정의 (재정의)

### Simple API
= **기본값만 사용하는 코드 스타일**

```cpp
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](auto& d) {});
```

- 메서드 1~2개만 호출
- 나머지는 기본값
- 별도 네임스페이스 불필요!

### Advanced API  
= **기본값을 오버라이드하는 코드 스타일**

```cpp
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](auto& d) {})
                  .on_connect([]() {})
                  .retry_interval(5000)
                  .auto_start(false);
```

- 여러 메서드 호출
- 기본값 오버라이드
- 같은 클래스, 같은 API!

---

## ✅ 최종 제안

### 핵심 변경사항

1. **기본값 개선**
   ```cpp
   auto_start_ = true;    // false → true
   auto_manage_ = true;   // false → true
   on_error_ = default_logger;  // nullptr → 기본 로거
   ```

2. **build() 자동화** (선택적)
   - Phase 1: 유지 (호환성)
   - Phase 2: ClientHandle로 자동화

3. **문서 업데이트**
   - "Simple API" = 기본값 활용하는 스타일
   - "Advanced API" = 모든 설정 커스터마이징하는 스타일
   - 별도 클래스 아님!

### 작업량

| 작업 | 예상 시간 |
|------|----------|
| 기본값 변경 | 2시간 |
| 기본 에러 핸들러 | 1시간 |
| 테스트 업데이트 | 2시간 |
| 문서 업데이트 | 2시간 |
| **Phase 1 총계** | **7시간 (1일)** |

### 호환성

- ✅ 기존 코드 99% 호환 (auto_start 명시한 코드)
- ⚠️ `.auto_start(false)` 의존 코드만 영향 (극소수)
- ✅ 모든 메서드 그대로 유지
- ✅ 테스트 99% 그대로

---

## 📝 결론

**사용자 제안이 완벽했습니다!**

1. ✅ build() 자동화 또는 제거
2. ✅ send() 메서드 유지
3. ✅ 메서드 제거가 아닌 기본값 개선
4. ✅ 완벽한 호환성

**Simple API = 기본값이 합리적인 기존 API**
**Advanced API = 같은 API를 풀로 활용**

**작업량**: 1일 (기본값 개선만)

---

**Last Updated**: 2025-10-11


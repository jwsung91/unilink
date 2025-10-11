# Unilink API Redesign - 호환성 검토 및 마이그레이션 플랜

**작성일**: 2025-10-11  
**상태**: 검토 중 (Review)  
**목표**: 공개 API를 단순화하고 Options 기반 설정으로 통합

---

## 📋 Executive Summary

현재 Unilink는 Builder 패턴 기반의 유연한 API를 제공하고 있습니다. 제안된 재설계는 이를 2단계 API(Simple/Advanced)로 단순화하고 단일 `Options` 구조체로 설정을 통합하려는 목표입니다.

**⚠️ 중요**: 이 재설계는 **Source Compatibility를 보장하지 않습니다**. Breaking Change입니다.

---

## 🔍 현재 상태 분석

### 현재 공개 API Surface

#### 1. **Builder API** (Primary Public Interface)
- **위치**: `unilink/builder/`
- **클래스들**:
  - `TcpServerBuilder`
  - `TcpClientBuilder` 
  - `SerialBuilder`
  - `UnifiedBuilder`
- **특징**: 
  - Fluent API로 메서드 체이닝
  - `build()` 메서드로 wrapper 인스턴스 생성
  - 콜백 설정 (`on_data`, `on_connect`, `on_disconnect`, `on_error`)
  - 자동화 옵션 (`auto_start`, `auto_manage`)

**사용 예시**:
```cpp
auto client = unilink::tcp_client("127.0.0.1", 8080)
    .on_connect([]() { ... })
    .on_data([](const std::string& data) { ... })
    .auto_start(true)
    .build();
```

#### 2. **Wrapper API** (Indirect Public Interface)
- **위치**: `unilink/wrapper/`
- **클래스들**:
  - `TcpServer`
  - `TcpClient`
  - `Serial`
- **인터페이스**: `ChannelInterface`
- **메서드들**:
  - `start()`, `stop()`
  - `send(const std::string&)`, `send_line(const std::string&)`
  - `is_connected()`
  - 이벤트 핸들러 설정

#### 3. **Config API** (Optional Feature)
- **위치**: `unilink/config/`
- **조건부 컴파일**: `UNILINK_ENABLE_CONFIG`
- **기능**: 설정 파일 기반 구성

#### 4. **Common Utilities**
- **위치**: `unilink/common/`
- **기능**: 로깅, 에러 처리, 유틸리티 함수

### 현재 디렉토리 구조
```
unilink/
├── builder/           # Builder 패턴 구현 (공개 API)
├── wrapper/           # Wrapper 클래스들 (Builder가 생성)
├── transport/         # 저수준 전송 계층 (내부 구현)
├── interface/         # 추상 인터페이스들
├── factory/           # 팩토리 패턴
├── config/            # 설정 관리 (선택적)
├── common/            # 공통 유틸리티
└── unilink.hpp        # 단일 진입점 헤더
```

### 핵심 의존성
- **Boost.Asio**: 비동기 I/O
- **C++17**: 표준 라이브러리
- **Threads**: 멀티스레딩

---

## 🎯 제안된 새 API Surface

### 새 구조
```
include/unilink/
├── link.hpp           # 단일 핸들 클래스
├── options.hpp        # 통합 설정 구조체
├── api.hpp            # Level 1: Simple API
└── api_ex.hpp         # Level 2: Advanced API

src/
├── link.cpp
├── options.cpp
├── api.cpp
└── api_ex.cpp
```

### 새 API 시그니처

#### Level 1 (Simple)
```cpp
// TCP
Link tcp_connect(std::string_view host, uint16_t port, RecvCB on_recv, ErrorCB on_error);
Link serial_open(std::string_view device, uint32_t baud, RecvCB on_recv, ErrorCB on_error);

// Link 사용
link.send(bytes);
link.stop();
link.is_open();
```

#### Level 2 (Advanced)
```cpp
// Options로 모든 설정 제어
Options opt;
opt.recv_timeout = std::chrono::milliseconds{1000};
opt.tcp.nodelay = true;
opt.on_recv = [](std::span<const std::byte> data) { ... };

Link tcp_connect_ex(std::string_view host, uint16_t port, const Options& opt);
```

---

## ⚠️ 호환성 영향 분석

### 1. Breaking Changes

#### 1.1 Builder API 제거
**현재**: 
```cpp
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](const std::string& data) { })
                  .build();
client->send("hello");
```

**새 API**:
```cpp
auto link = unilink::tcp_connect("host", 8080,
    [](std::span<const std::byte> data) { },
    [](std::error_code ec) { });
link.send(std::as_bytes(std::span("hello")));
```

**영향**:
- ✗ 메서드 체이닝 불가능
- ✗ `build()` 호출 필요 없음 (바로 Link 반환)
- ✗ 콜백 시그니처 변경 (`std::string` → `std::span<const std::byte>`)
- ✗ 반환 타입 변경 (`unique_ptr<TcpClient>` → `Link` by value)

#### 1.2 데이터 타입 변경
**현재**: `std::string` 기반
**새로운**: `std::span<const std::byte>` 기반

**영향**:
- 모든 콜백과 send 메서드가 영향 받음
- 사용자 코드의 타입 변환 필요

#### 1.3 인터페이스 변경
**현재**: Wrapper 클래스들 (`TcpClient`, `Serial`, `TcpServer`)
**새로운**: 단일 `Link` 클래스

**영향**:
- 타입 의존 코드 수정 필요
- Wrapper 특화 메서드 접근 불가

#### 1.4 에러 처리 변경
**현재**: 
- 콜백: `on_error(const std::string& error)`
- 예외 사용 가능

**새로운**:
- 콜백: `on_error(std::error_code ec)`
- `send()` 반환: `std::error_code`
- No-throw 보장

### 2. 영향받는 코드 범위

#### 2.1 Examples (4개 파일)
- ✗ `examples/tutorials/getting_started/simple_client.cpp`
- ✗ `examples/tutorials/getting_started/my_first_client.cpp`
- ✗ `examples/tutorials/tcp_server/chat_server.cpp`
- ✗ `examples/tutorials/tcp_server/echo_server.cpp`

#### 2.2 Tests (34개 파일)
모든 테스트 파일이 Builder API 또는 Wrapper API를 사용:
- Unit tests: 13개
- Integration tests: 11개
- E2E tests: 3개
- Performance tests: 7개

**특히 영향 큰 테스트들**:
- `test_builder.cc`
- `test_tcp_client_advanced.cc`
- `test_tcp_server_advanced.cc`
- `test_builder_integration.cc`

#### 2.3 Documentation
- README.md (Quick Start 예제)
- API_GUIDE.md
- 각종 튜토리얼

### 3. 내부 구현 재사용 가능성

**좋은 소식**: 내부 Transport 계층은 대부분 재사용 가능
- `transport/tcp_client/`
- `transport/tcp_server/`
- `transport/serial/`
- `interface/channel.hpp`

**필요한 작업**:
- 새 `Link::Impl`이 기존 transport를 wrapping
- `Options`를 기존 config 구조체로 변환하는 adaptor

---

## 🚨 주요 우려사항

### 1. API 단순성 vs 유연성 Trade-off

**현재 Builder의 장점**:
- ✅ 메서드 체이닝으로 가독성 좋음
- ✅ 선택적 설정 (필요한 것만 호출)
- ✅ IDE 자동완성 친화적
- ✅ 타입 안전성 (컴파일 타임 체크)

**새 API의 장점**:
- ✅ 공개 헤더 수 감소 (4개만)
- ✅ 간단한 사용 케이스에서 더 짧은 코드
- ✅ 함수형 스타일

**새 API의 단점**:
- ✗ Advanced 설정시 Options 구조체 복잡도 증가
- ✗ 모든 콜백을 한 곳에 정의해야 함 (분산 불가)
- ✗ Level 1과 Level 2의 일관성 부족

### 2. std::span<const std::byte> 사용의 영향

**장점**:
- ✅ 바이너리 데이터 처리 명확
- ✅ 복사 없는 view

**단점**:
- ✗ C++20 feature (현재는 C++17)
  - C++17에서는 gsl::span 등 써드파티 필요
  - 또는 `const uint8_t*, size_t` pair 사용
- ✗ 문자열 처리가 더 불편 (string_view와 호환 안됨)
- ✗ 사용자가 타입 변환 부담

### 3. Link 값 의미론 (Value Semantics)

제안서는 `Link(Link&&)` move-only 타입을 제안하는데:

**우려사항**:
- 기존 `unique_ptr<TcpClient>` 패턴이 더 명확
- RAII와 소유권이 이미 잘 표현됨
- Move-only value type은 함수 리턴에서 RVO 의존

### 4. 기존 사용자에 대한 영향

**Breaking Change**로 인한 리스크:
- 현재 사용자 전원이 코드 수정 필요
- 마이그레이션 가이드 필수
- 일부 사용자는 업그레이드 거부 가능

---

## 📊 대안 검토

### 대안 1: Builder API 유지 + 단순 API 추가
**접근**: 기존 Builder 유지하면서 Simple API를 추가

```cpp
// 기존 (유지)
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](const std::string& data) { })
                  .build();

// 새 Simple API (추가)
namespace unilink::simple {
    Link tcp_connect(string_view host, uint16_t port, 
                     RecvCB on_recv, ErrorCB on_error);
}
```

**장점**:
- ✅ 기존 코드 호환성 유지
- ✅ 새 사용자에게 더 간단한 API 제공
- ✅ 점진적 마이그레이션 가능

**단점**:
- ✗ API surface 증가
- ✗ 두 API 유지보수 부담

### 대안 2: Builder 개선 (Options 통합)
**접근**: Builder 패턴 유지하되 Options로 일괄 설정 가능하게

```cpp
// 기존 방식 (유지)
auto client = unilink::tcp_client("host", 8080)
                  .on_data([](const std::string& data) { })
                  .build();

// Options 방식 (추가)
Options opt;
opt.tcp.nodelay = true;
opt.on_recv = [](const std::string& data) { };

auto client = unilink::tcp_client("host", 8080)
                  .with_options(opt)
                  .build();
```

**장점**:
- ✅ 기존 API 유지
- ✅ Options 기반 설정도 지원
- ✅ 유연성 최대

**단점**:
- ✗ 두 방식 혼재 가능 (혼란)

### 대안 3: 제안대로 진행 (Breaking Change 수용)
**접근**: 제안서대로 완전히 새로운 API로 교체

**장점**:
- ✅ 깔끔한 API
- ✅ 공개 헤더 최소화

**단점**:
- ✗ 모든 사용자 코드 수정 필요
- ✗ 높은 마이그레이션 비용

---

## 🎯 권장 접근 방식

### 추천: 단계별 하이브리드 전략

#### Phase 1: Simple API 추가 (호환성 유지)
1. 새로운 `unilink/simple/` 네임스페이스 추가
2. 기존 Builder 유지
3. Simple API를 기존 Builder로 구현
4. 예제와 문서 추가

```cpp
namespace unilink::simple {
    // 내부적으로 Builder 사용
    inline std::unique_ptr<wrapper::TcpClient> 
    tcp_connect(string_view host, uint16_t port,
                std::function<void(const std::string&)> on_recv,
                std::function<void(const std::string&)> on_error) {
        return tcp_client(std::string(host), port)
            .on_data(std::move(on_recv))
            .on_error(std::move(on_error))
            .auto_start(true)
            .build();
    }
}
```

#### Phase 2: Options 통합 (호환성 유지)
1. `Options` 구조체 도입
2. Builder에 `.with_options(opt)` 메서드 추가
3. 내부적으로 Options를 Builder 설정으로 변환

#### Phase 3: 평가 및 결정
1. 새 API 사용 피드백 수집
2. 필요시 Phase 4 진행
3. 그렇지 않으면 현재 상태 유지

#### Phase 4 (선택적): Deprecation 및 새 API로 전환
1. 기존 Builder API deprecated 표시
2. 마이그레이션 가이드 작성
3. Major version bump (v2.0)
4. 제안서대로 새 API로 교체

---

## 📝 실행 계획 (Phase 1 기준)

### Step 1: 기반 작업
- [ ] `include/unilink/simple/` 디렉토리 생성
- [ ] `link.hpp` 작성 (wrapper의 type alias 또는 thin wrapper)
- [ ] `options.hpp` 작성 (기본 구조체)

### Step 2: Simple API 구현
- [ ] `simple/api.hpp` 작성
  - `tcp_connect()` 구현
  - `serial_open()` 구현
- [ ] 내부적으로 기존 Builder 활용
- [ ] 단위 테스트 작성

### Step 3: 문서화
- [ ] Simple API 사용 가이드
- [ ] 예제 코드 추가
- [ ] README에 Simple API 소개

### Step 4: 검증
- [ ] 기존 테스트 모두 통과 확인
- [ ] 새 Simple API 테스트
- [ ] 성능 영향 평가

### Step 5: 릴리즈
- [ ] Minor version bump (v1.x.0)
- [ ] Changelog 작성
- [ ] 커뮤니티 공지

---

## ⏱️ 예상 작업량

### Phase 1 (Simple API 추가)
- **개발**: 2-3일
- **테스트**: 1-2일
- **문서화**: 1일
- **총**: 4-6일

### Full Redesign (제안서대로)
- **설계 및 구현**: 5-7일
- **테스트 전면 수정**: 3-5일
- **예제 및 문서 수정**: 2-3일
- **총**: 10-15일

---

## 🤔 결정 필요 사항

### 질문 1: Breaking Change 수용 가능한가?
- 현재 사용자 베이스 크기?
- 버전 정책? (SemVer 준수?)
- v2.0 출시 계획 있는가?

### 질문 2: std::span 사용 여부
- C++20으로 업그레이드 가능한가?
- 아니면 C++17에서 대안? (gsl::span, 또는 ptr+size)

### 질문 3: 우선순위
- **옵션 A**: 단순성 > 호환성 → 제안서대로 진행
- **옵션 B**: 호환성 > 단순성 → 단계별 접근
- **옵션 C**: 현상 유지 → Builder API 개선만

### 질문 4: Timeline
- 언제까지 완료 필요?
- 다른 feature와의 우선순위?

---

## 📋 체크리스트 (제안서 진행시)

만약 제안서대로 Full Redesign을 진행한다면:

### 설계 단계
- [ ] std::span 대신 C++17 호환 대안 결정
- [ ] Link 소유권 모델 명확화
- [ ] Options 구조체 상세 설계
- [ ] 에러 코드 체계 설계

### 구현 단계
- [ ] `Link` 클래스 및 pimpl 구현
- [ ] `Options` 구조체 구현
- [ ] Level 1 Simple API 구현
- [ ] Level 2 Advanced API 구현
- [ ] 기존 transport 계층 adaptor 구현

### 마이그레이션 단계
- [ ] 기존 Builder API deprecated
- [ ] 마이그레이션 스크립트/도구 작성
- [ ] 34개 테스트 파일 전부 수정
- [ ] 4개 예제 파일 수정
- [ ] 모든 문서 업데이트

### 검증 단계
- [ ] 모든 테스트 통과
- [ ] 성능 회귀 없음 확인
- [ ] 메모리 안전성 검증
- [ ] CI/CD 통과

### 릴리즈 단계
- [ ] CHANGELOG 작성
- [ ] 마이그레이션 가이드 작성
- [ ] Major version bump (v2.0.0)
- [ ] 커뮤니티 공지 및 지원

---

## 💡 최종 권장사항

**권장**: **Phase 1 (Simple API 추가)부터 시작**

**이유**:
1. **위험 최소화**: 기존 코드 호환성 유지
2. **점진적 개선**: 사용자 피드백 기반 개선 가능
3. **빠른 가치 제공**: 단순 사용 케이스에 바로 적용 가능
4. **유연성**: 나중에 Full Redesign 선택 가능

**다음 단계**:
1. 이 문서를 팀/커뮤니티와 리뷰
2. Breaking Change 수용 가능 여부 결정
3. Phase 1 구현 시작 또는 Full Redesign 진행 결정

---

## 📞 연락처

질문이나 피드백: [프로젝트 이슈 트래커]

**Last Updated**: 2025-10-11


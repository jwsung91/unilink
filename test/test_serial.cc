#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>

#include "unilink/interface/iserial_port.hpp"
#include "unilink/transport/serial/serial.hpp"

using namespace unilink::transport;
using namespace unilink::config;
using namespace unilink::common;
using namespace unilink::interface;
namespace net = boost::asio;

using ::testing::_;
using ::testing::A;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgReferee;

class MockSerialPort : public ISerialPort {
 public:
  MOCK_METHOD(void, open, (const std::string&, boost::system::error_code&),
              (override));
  MOCK_METHOD(bool, is_open, (), (const, override));
  MOCK_METHOD(void, close, (boost::system::error_code&), (override));

  MOCK_METHOD(void, set_option,
              (const net::serial_port_base::baud_rate&,
               boost::system::error_code&),
              (override));
  MOCK_METHOD(void, set_option,
              (const net::serial_port_base::character_size&,
               boost::system::error_code&),
              (override));
  MOCK_METHOD(void, set_option,
              (const net::serial_port_base::stop_bits&,
               boost::system::error_code&),
              (override));
  MOCK_METHOD(void, set_option,
              (const net::serial_port_base::parity&,
               boost::system::error_code&),
              (override));
  MOCK_METHOD(void, set_option,
              (const net::serial_port_base::flow_control&,
               boost::system::error_code&),
              (override));

  MOCK_METHOD(void, async_read_some,
              (const net::mutable_buffer&,
               std::function<void(const boost::system::error_code&, size_t)>),
              (override));
  MOCK_METHOD(void, async_write,
              (const net::const_buffer&,
               std::function<void(const boost::system::error_code&, size_t)>),
              (override));
};

class SerialTest : public ::testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {
    if (serial_) serial_->stop();
    if (ioc_thread_.joinable()) {
      ioc_thread_.join();
    }
  }

  SerialConfig cfg_;
  net::io_context test_ioc_;
  std::thread ioc_thread_;
  MockSerialPort* mock_port_;  // Non-owning, valid for the test's lifetime
  std::shared_ptr<Serial> serial_;

  std::mutex mtx_;
  std::condition_variable cv_;
};

TEST_F(SerialTest, ConnectsAndReceivesStateCallback) {
  // --- Setup ---
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  // --- Expectations ---
  EXPECT_CALL(*mock_port_, open(_, _))
      .WillOnce(SetArgReferee<1>(boost::system::error_code()));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::character_size&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::stop_bits&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::parity&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::flow_control&>(), _));

  // Keep the read loop alive
  EXPECT_CALL(*mock_port_, async_read_some(_, _))
      .WillRepeatedly([](auto, auto) { /* Keep the read loop going */ });

  // --- Test Logic ---
  std::unique_lock<std::mutex> lock(mtx_);
  LinkState received_state = LinkState::Idle;
  int state_cb_count = 0;
  serial_->on_state([&](LinkState state) {
    std::lock_guard<std::mutex> lock_guard(mtx_);
    received_state = state;
    state_cb_count++;
    cv_.notify_one();
  });
  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });

  // Connecting, Connected 두 번의 콜백을 기다림
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1),
                           [&] { return state_cb_count >= 2; }));

  // --- Verification ---
  EXPECT_EQ(received_state, LinkState::Connected);
}

TEST_F(SerialTest, ReceivesData) {
  // --- Setup ---
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  // --- Expectations ---
  const std::string test_message = "hello";
  std::function<void(const boost::system::error_code&, size_t)> read_handler;
  net::mutable_buffer read_buffer;

  EXPECT_CALL(*mock_port_, open(_, _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::character_size&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::stop_bits&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::parity&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::flow_control&>(), _));
  EXPECT_CALL(*mock_port_, async_read_some(_, _))
      // The first call will save the handler.
      .WillOnce(DoAll(SaveArg<0>(&read_buffer), SaveArg<1>(&read_handler)))
      // Subsequent recursive calls will do nothing, keeping the loop alive
      // without re-triggering the test logic.
      .WillRepeatedly(Return());

  // --- Test Logic ---
  std::unique_lock<std::mutex> lock(mtx_);
  std::vector<uint8_t> received_data;
  LinkState current_state = LinkState::Idle;

  // Set callbacks before starting
  serial_->on_bytes([&](const uint8_t* data, size_t n) {
    std::lock_guard<std::mutex> lock_guard(mtx_);
    received_data.insert(received_data.end(), data, data + n);
    cv_.notify_one();
  });
  serial_->on_state([&](LinkState state) {
    std::lock_guard<std::mutex> lock_guard(mtx_);
    current_state = state;
    cv_.notify_one();
  });

  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });

  // Wait until the serial port is connected and ready to read.
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1), [&] {
    return current_state == LinkState::Connected;
  }));
  ASSERT_TRUE(read_handler);

  // Simulate data arrival by copying data to the buffer and invoking the
  // handler
  ASSERT_GE(read_buffer.size(), test_message.length());
  std::memcpy(read_buffer.data(), test_message.data(), test_message.length());
  net::post(test_ioc_, [&] {
    read_handler(boost::system::error_code(), test_message.length());
  });

  // Wait for the on_bytes callback to be fired.
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1),
                           [&] { return !received_data.empty(); }));

  // --- Verification ---
  std::string received_str(received_data.begin(), received_data.end());
  EXPECT_EQ(received_str, test_message);
}

TEST_F(SerialTest, TransmitsData) {
  // --- Setup ---
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  // --- Expectations ---
  std::function<void(const boost::system::error_code&, size_t)> write_handler;
  net::const_buffer written_buffer;
  EXPECT_CALL(*mock_port_, open(_, _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::character_size&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::stop_bits&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::parity&>(), _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::flow_control&>(), _));
  EXPECT_CALL(*mock_port_, async_read_some(_, _))
      .WillRepeatedly(Return());  // Keep the read loop alive
  EXPECT_CALL(*mock_port_, async_write(_, _))
      .WillOnce(DoAll(SaveArg<0>(&written_buffer), Return()));

  // --- Test Logic ---
  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });
  const std::string test_message = "world";
  serial_->async_write_copy(
      reinterpret_cast<const uint8_t*>(test_message.c_str()),
      test_message.length());

  // --- Verification ---
  // Wait until async_write is called and the buffer is captured.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  ASSERT_EQ(written_buffer.size(), test_message.size());
  std::string written_str(static_cast<const char*>(written_buffer.data()),
                          written_buffer.size());
  EXPECT_EQ(written_str, test_message);
}

TEST_F(SerialTest, FutureInCallbackDoesNotBlockIoContext) {
  // --- Setup ---
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  // --- Expectations ---
  // This test verifies that a blocking operation (like future::wait) inside a
  // completion handler does NOT block the io_context thread. This is because
  // the handler is executed by the io_context, but the wait should happen on a
  // different thread. The Serial class itself doesn't use futures, but this
  // test simulates a user doing so in a callback.

  std::function<void(const boost::system::error_code&, size_t)> write_handler1;
  std::function<void(const boost::system::error_code&, size_t)> write_handler2;

  EXPECT_CALL(*mock_port_, open(_, _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_, async_read_some(_, _)).WillRepeatedly(Return());
  EXPECT_CALL(*mock_port_, async_write(_, _))
      .WillOnce(SaveArg<1>(&write_handler1))
      .WillOnce(SaveArg<1>(&write_handler2));

  // --- Test Logic ---
  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });

  // 1. First write. The handler will be captured.
  const std::string msg1 = "first";
  serial_->async_write_copy(reinterpret_cast<const uint8_t*>(msg1.c_str()),
                            msg1.length());

  // Wait until the first async_write is called.
  while (!write_handler1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 2. Second write. This should be queued behind the first one.
  const std::string msg2 = "second";
  serial_->async_write_copy(reinterpret_cast<const uint8_t*>(msg2.c_str()),
                            msg2.length());

  // 3. Simulate completion of the first write. Inside the handler, we'll
  //    block using a future to see if it freezes the io_context.
  std::promise<void> p;
  auto fut = p.get_future();
  net::post(test_ioc_, [&] {
    write_handler1(boost::system::error_code(), msg1.length());
    p.set_value();  // Unblock the main thread
  });

  // 4. Verification: If the io_context was blocked, the second write would
  //    never be initiated, and write_handler2 would remain null.
  EXPECT_EQ(fut.wait_for(std::chrono::seconds(1)), std::future_status::ready);
  EXPECT_TRUE(write_handler2);
}

TEST_F(SerialTest, HandlesConnectionFailureAndRetries) {
  // --- Setup ---
  cfg_.reopen_on_error = true;
  cfg_.retry_interval_ms = 50;  // 짧은 재시도 간격
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  // --- Expectations ---
  // 첫 번째 open 시도는 실패, 두 번째는 성공
  EXPECT_CALL(*mock_port_, open(_, _))
      .WillOnce(SetArgReferee<1>(
          boost::asio::error::make_error_code(boost::asio::error::not_found)))
      .WillOnce(SetArgReferee<1>(boost::system::error_code()));

  // 성공적인 연결 후 설정 호출 예상
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::character_size&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::stop_bits&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::parity&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::flow_control&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_, async_read_some(_, _)).WillRepeatedly(Return());

  // --- Test Logic ---
  std::unique_lock<std::mutex> lock(mtx_);
  std::vector<LinkState> states;
  serial_->on_state([&](LinkState state) {
    std::lock_guard<std::mutex> lock_guard(mtx_);
    states.push_back(state);
    cv_.notify_one();
  });

  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });

  // 상태 변화를 기다림: Connecting -> (실패 후 다시) Connecting -> Connected
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1),
                           [&] { return states.size() >= 3; }));

  // --- Verification ---
  // 최종 상태는 Connected 여야 함
  EXPECT_EQ(states.back(), LinkState::Connected);
  // 첫 상태는 Connecting
  EXPECT_EQ(states.front(), LinkState::Connecting);
}

TEST_F(SerialTest, HandlesWriteError) {
  // --- Setup ---
  cfg_.reopen_on_error = false;  // 재시도 없이 Error 상태로 가는지 확인
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);
  // --- Expectations ---
  std::function<void(const boost::system::error_code&, size_t)> write_handler;
  EXPECT_CALL(*mock_port_, open(_, _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_, async_read_some(_, _)).WillRepeatedly(Return());
  EXPECT_CALL(*mock_port_, is_open()).WillRepeatedly(Return(true));
  EXPECT_CALL(*mock_port_, close(_));  // 에러 발생 시 close가 호출되어야 함

  // async_write가 호출되면 핸들러를 저장하고, 에러를 발생시킬 준비
  EXPECT_CALL(*mock_port_, async_write(_, _))
      .WillOnce(SaveArg<1>(&write_handler));

  // --- Test Logic ---
  std::unique_lock<std::mutex> lock(mtx_);
  LinkState current_state = LinkState::Idle;
  serial_->on_state([&](LinkState state) {
    std::lock_guard<std::mutex> lock_guard(mtx_);
    current_state = state;
    cv_.notify_one();
  });

  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });

  // Connected 상태가 될 때까지 대기
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1), [&] {
    return current_state == LinkState::Connected;
  }));

  const std::string msg = "test";
  serial_->async_write_copy(reinterpret_cast<const uint8_t*>(msg.c_str()),
                            msg.length());

  // write_handler가 캡처될 때까지 잠시 대기
  while (!write_handler) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 쓰기 에러 시뮬레이션
  net::post(test_ioc_, [&] {
    write_handler(
        boost::asio::error::make_error_code(boost::asio::error::broken_pipe),
        0);
  });

  // 최종 상태가 Error가 될 때까지 대기
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1),
                           [&] { return current_state == LinkState::Error; }));

  // --- Verification ---
  EXPECT_EQ(current_state, LinkState::Error);
}

TEST_F(SerialTest, QueuesMultipleWrites) {
  // --- Setup ---
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  // --- Expectations ---
  EXPECT_CALL(*mock_port_, open(_, _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_, async_read_some(_, _)).WillRepeatedly(Return());
  // async_write가 두 번 호출될 것을 기대
  EXPECT_CALL(*mock_port_, async_write(_, _))
      .Times(2)
      .WillRepeatedly(Invoke([this](auto, auto handler) {
        // 쓰기 완료를 즉시 시뮬레이션하여 다음 쓰기가 시작되도록 함
        net::post(test_ioc_,
                  [handler]() { handler(boost::system::error_code(), 0); });
      }));

  // --- Test Logic ---
  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });
  std::this_thread::sleep_for(
      std::chrono::milliseconds(100));  // start()가 처리될 시간

  const uint8_t data1[] = {0x01};
  const uint8_t data2[] = {0x02};
  serial_->async_write_copy(data1, 1);
  serial_->async_write_copy(data2, 2);

  std::this_thread::sleep_for(
      std::chrono::milliseconds(100));  // 모든 쓰기가 처리될 시간
}

TEST_F(SerialTest, FutureWaitSucceedsWithinTimeout) {
  // --- Setup ---
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  const std::string test_message = "data";
  std::function<void(const boost::system::error_code&, size_t)> read_handler;
  net::mutable_buffer read_buffer;

  // --- Expectations ---
  EXPECT_CALL(*mock_port_, open(_, _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _))
      .Times(AtLeast(1));
  EXPECT_CALL(*mock_port_, async_read_some(_, _))
      .WillOnce(DoAll(SaveArg<0>(&read_buffer), SaveArg<1>(&read_handler)))
      .WillRepeatedly(Return());

  // --- Test Logic ---
  std::promise<std::vector<uint8_t>> data_promise;
  auto data_future = data_promise.get_future();

  serial_->on_bytes([&](const uint8_t* data, size_t n) {
    data_promise.set_value(std::vector<uint8_t>(data, data + n));
  });

  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });

  // read_handler가 캡처될 때까지 대기
  while (!read_handler) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 2초 후에 데이터 수신을 시뮬레이션하는 별도 스레드
  std::thread sim_thread([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ASSERT_GE(read_buffer.size(), test_message.length());
    std::memcpy(read_buffer.data(), test_message.data(), test_message.length());
    net::post(test_ioc_, [&] {
      read_handler(boost::system::error_code(), test_message.length());
    });
  });

  // 3초 타임아웃으로 future를 기다림
  auto status = data_future.wait_for(std::chrono::seconds(3));

  // --- Verification ---
  // 2초 후에 promise가 set_value() 되므로, 3초 타임아웃 내에 future는 ready
  // 상태가 되어야 함
  ASSERT_EQ(status, std::future_status::ready);
  auto received_data = data_future.get();
  std::string received_str(received_data.begin(), received_data.end());
  EXPECT_EQ(received_str, test_message);

  if (sim_thread.joinable()) {
    sim_thread.join();
  }
}

TEST_F(SerialTest, FutureWaitTimesOut) {
  // --- Setup ---
  auto mock_port_ptr = std::make_unique<MockSerialPort>();
  mock_port_ = mock_port_ptr.get();
  serial_ = std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);

  // --- Expectations ---
  EXPECT_CALL(*mock_port_, open(_, _));
  EXPECT_CALL(*mock_port_,
              set_option(A<const net::serial_port_base::baud_rate&>(), _))
      .Times(AtLeast(1));
  // 데이터 수신 시뮬레이션을 하지 않으므로, read_handler는 호출되지 않음
  EXPECT_CALL(*mock_port_, async_read_some(_, _)).WillRepeatedly(Return());

  // --- Test Logic ---
  std::promise<void> timeout_promise;
  auto timeout_future = timeout_promise.get_future();

  // on_bytes 콜백이 호출되면 promise를 fulfill 하지만, 이 테스트에서는 호출되지
  // 않음
  serial_->on_bytes(
      [&](const uint8_t* data, size_t n) { timeout_promise.set_value(); });

  serial_->start();
  ioc_thread_ = std::thread([this] { test_ioc_.run(); });

  // 3초 타임아웃으로 future를 기다림
  auto status = timeout_future.wait_for(std::chrono::seconds(3));

  // --- Verification ---
  // 데이터 수신이 없으므로, 3초 후에 future는 timeout 상태가 되어야 함
  EXPECT_EQ(status, std::future_status::timeout);
}
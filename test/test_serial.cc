#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "pty_helper.hpp"
#include "unilink/transport/serial/serial.hpp"

using namespace unilink::transport;
using namespace unilink::config;
using namespace unilink::common;

class SerialTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // pty 헬퍼를 사용하여 가상 시리얼 포트 생성
    pty_ = std::make_unique<PtyHelper>();
    pty_->init();

    // SerialConfig 설정
    cfg_.device = pty_->slave_name();
    cfg_.baud_rate = 9600;         // pty에서는 baud_rate가 큰 의미는 없음
    cfg_.reopen_on_error = false;  // 테스트에서는 재시도 비활성화

    serial_ = std::make_shared<Serial>(cfg_);
  }

  void TearDown() override {
    serial_->stop();
    pty_.reset();
  }

  std::unique_ptr<PtyHelper> pty_;
  SerialConfig cfg_;
  std::shared_ptr<Serial> serial_;

  std::mutex mtx_;
  std::condition_variable cv_;
};

TEST_F(SerialTest, ConnectsAndReceivesStateCallback) {
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

  // Connecting, Connected 두 번의 콜백을 기다림
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1),
                           [&] { return state_cb_count >= 2; }));

  EXPECT_EQ(received_state, LinkState::Connected);
  EXPECT_TRUE(serial_->is_connected());
}

TEST_F(SerialTest, ReceivesData) {
  std::unique_lock<std::mutex> lock(mtx_);
  std::vector<uint8_t> received_data;

  serial_->on_bytes([&](const uint8_t* data, size_t n) {
    std::lock_guard<std::mutex> lock_guard(mtx_);
    received_data.insert(received_data.end(), data, data + n);
    cv_.notify_one();
  });

  serial_->start();
  // 연결될 때까지 잠시 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // pty 마스터를 통해 데이터 전송
  const std::string test_message = "hello serial";
  ssize_t bytes_written =
      write(pty_->master_fd(), test_message.c_str(), test_message.length());
  ASSERT_EQ(bytes_written, test_message.length());

  // 데이터 수신을 기다림
  ASSERT_TRUE(cv_.wait_for(lock, std::chrono::seconds(1),
                           [&] { return !received_data.empty(); }));

  std::string received_str(received_data.begin(), received_data.end());
  EXPECT_EQ(received_str, test_message);
}

TEST_F(SerialTest, TransmitsData) {
  serial_->start();
  // 연결될 때까지 잠시 대기
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  const std::string test_message = "world";
  serial_->async_write_copy(
      reinterpret_cast<const uint8_t*>(test_message.c_str()),
      test_message.length());

  // pty 마스터에서 데이터 읽기
  char read_buf[128];
  // non-blocking read를 위해 잠시 대기 후 읽기
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ssize_t bytes_read = read(pty_->master_fd(), read_buf, sizeof(read_buf) - 1);

  ASSERT_GT(bytes_read, 0);
  read_buf[bytes_read] = '\0';
  EXPECT_EQ(std::string(read_buf), test_message);
}
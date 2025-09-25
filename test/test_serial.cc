#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
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
  void SetUp() override {
    auto mock_port_ptr = std::make_unique<MockSerialPort>();
    mock_port_ = mock_port_ptr.get();  // Save raw pointer for expectations

    // Use the test-specific constructor to inject the mock port and io_context
    serial_ =
        std::make_shared<Serial>(cfg_, std::move(mock_port_ptr), test_ioc_);
  }

  void TearDown() override {
    serial_->stop();
    if (ioc_thread_.joinable()) ioc_thread_.join();
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
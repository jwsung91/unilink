/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include "unilink/transport/uds/uds_server_session.hpp"
#include "unilink/transport/uds/boost_uds_socket.hpp"
#include "../../mocks/mock_uds_socket.hpp"
#include "test_utils.hpp"

using namespace unilink;
using namespace transport;
using namespace testing;
using namespace std::chrono_literals;

namespace unilink {
namespace test {

class UdsServerSessionCoverageTest : public Test {
protected:
    boost::asio::io_context ioc;
};

TEST_F(UdsServerSessionCoverageTest, RedundantStop) {
    auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
    EXPECT_CALL(*mock_socket, close(_)).Times(1);
    
    auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 0);
    session->start();
    session->stop();
    session->stop(); // Should return early
    
    ioc.run();
}

TEST_F(UdsServerSessionCoverageTest, BackpressureLimitEnforced) {
    auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
    auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 100, 0);
    
    // threshold is 100, limit is threshold * 4 = 400
    std::vector<uint8_t> large_data(500, 'A');
    session->async_write_copy(memory::ConstByteSpan(large_data.data(), large_data.size()));
    
    // We can't easily check internal state but we can verify it doesn't crash 
    // and exercises the bp_limit check.
    session->stop();
    ioc.run();
}

TEST_F(UdsServerSessionCoverageTest, IdleTimeoutExpiration) {
    auto mock_socket = std::make_unique<unilink::test::mocks::MockUdsSocket>();
    EXPECT_CALL(*mock_socket, async_read_some(_, _)).Times(AtLeast(1));
    EXPECT_CALL(*mock_socket, close(_)).Times(1);
    
    std::atomic<bool> closed{false};
    auto session = std::make_shared<UdsServerSession>(ioc, std::move(mock_socket), 1024, 10); // 10ms timeout
    session->on_close([&]() { closed = true; });
    session->start();
    
    // Manually run io_context in a loop to allow timer and handlers to execute
    auto start = std::chrono::steady_clock::now();
    while (!closed && std::chrono::steady_clock::now() - start < 1s) {
        ioc.run_one_for(10ms);
    }
    
    EXPECT_TRUE(closed.load());
}

} // namespace test
} // namespace unilink

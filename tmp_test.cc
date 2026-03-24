TEST_F(TransportTcpClientPolicyTest, ExponentialBackoffPolicyIncreasesDelay) {
  boost::asio::io_context ioc;
  config::TcpClientConfig cfg;
  cfg.host = "127.0.0.1";
  cfg.port = TestUtils::getAvailableTestPort();
  cfg.connection_timeout_ms = 20;

  client_ = TcpClient::create(cfg, ioc);

  client_->set_reconnect_policy(ExponentialBackoff(20ms, 1000ms, 2.0, false));

  std::vector<std::chrono::steady_clock::time_point> attempt_times;

  client_->on_state([&](base::LinkState state) {
    if (state == base::LinkState::Connecting) {
      attempt_times.push_back(std::chrono::steady_clock::now());
    }
  });

  client_->start();

  ioc.run_for(std::chrono::milliseconds(500));

  // Prevent callback accessing destroyed attempt_times
  client_->on_state(nullptr);
  client_->stop();

  // Debounce attempt times to filter out rapid-fire Connecting states (e.g. from handle_close -> schedule_retry)
  std::vector<std::chrono::steady_clock::time_point> filtered_times;
  if (!attempt_times.empty()) {
    filtered_times.push_back(attempt_times[0]);
    for (size_t i = 1; i < attempt_times.size(); ++i) {
      if (attempt_times[i] - attempt_times[i - 1] > std::chrono::milliseconds(10)) {
        filtered_times.push_back(attempt_times[i]);
      }
    }
  }

  EXPECT_GE(filtered_times.size(), 3);

  if (filtered_times.size() >= 3) {
    auto d1 = std::chrono::duration_cast<std::chrono::milliseconds>(filtered_times[1] - filtered_times[0]).count();
    auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(filtered_times[2] - filtered_times[1]).count();

    EXPECT_GT(d2, d1);
  }
  client_.reset();
}

TEST_F(TransportTcpClientPolicyTest, PolicyCanStopRetries) {
  boost::asio::io_context ioc;

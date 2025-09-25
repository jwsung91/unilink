#pragma once

#include <fcntl.h>
#include <gtest/gtest.h>
#include <termios.h>
#include <unistd.h>

#include <string>

// NOTE: This helper is for Linux/macOS only.
class PtyHelper {
 public:
  PtyHelper() = default;

  void init() {
    // Open a new pseudo-terminal master
    master_fd_ = posix_openpt(O_RDWR | O_NOCTTY);
    ASSERT_NE(master_fd_, -1)
        << "Failed to open pseudo-terminal master: " << strerror(errno);

    // Grant access to the slave pseudo-terminal
    ASSERT_EQ(grantpt(master_fd_), 0) << "grantpt failed: " << strerror(errno);

    // Unlock the slave pseudo-terminal
    ASSERT_EQ(unlockpt(master_fd_), 0)
        << "unlockpt failed: " << strerror(errno);

    // Get the name of the slave pseudo-terminal
    char* slave_name_ptr = ptsname(master_fd_);
    ASSERT_NE(slave_name_ptr, nullptr) << "ptsname failed: " << strerror(errno);
    slave_name_ = slave_name_ptr;
  }
  ~PtyHelper() {
    if (master_fd_ != -1) {
      close(master_fd_);
    }
  }

  int master_fd() const { return master_fd_; }
  const std::string& slave_name() const { return slave_name_; }

 private:
  int master_fd_ = -1;
  std::string slave_name_;
};
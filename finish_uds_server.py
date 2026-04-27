import os
import re

path = 'unilink/wrapper/uds_server/uds_server.cc'
with open(path, 'r') as f:
    content = f.read()

# Add bp_mutex_ and bp_cv_ to Impl
if 'std::condition_variable bp_cv_' not in content:
    content = re.sub(r'(std::shared_mutex\s+mutex_;)', r'\1\n  std::mutex bp_mutex_;\n  std::condition_variable bp_cv_;', content)

# Add send_to_blocking to Impl
if 'bool send_to_blocking(ClientId client_id, std::string_view data)' not in content:
    method = """
  bool send_to_blocking(ClientId client_id, std::string_view data) {
    std::unique_lock<std::mutex> lock(bp_mutex_);
    bp_cv_.wait(lock, [this, client_id]() {
      std::shared_lock<std::shared_mutex> rlock(mutex_);
      return !server_ || !server_->is_backpressure_active(client_id);
    });
    return send_to(client_id, data);
  }
"""
    content = re.sub(r'(bool\s+send_to\s*\([^)]+\)\s*\{[^}]+(?:\}[^}]+)*\n\s*\})', r'\1\n' + method, content, count=1)

# Hook on_backpressure in Impl
if 'bp_cv_.notify_all()' not in content:
    content = re.sub(r'(transport_server->on_backpressure\([^{]+\{)', r'\1\n        bp_cv_.notify_all();', content)

# Add send_to_blocking to UdsServer class
if 'bool UdsServer::send_to_blocking' not in content:
    method = """
bool UdsServer::send_to_blocking(ClientId client_id, std::string_view data) {
  return impl_->send_to_blocking(client_id, data);
}
"""
    content = re.sub(r'(bool\s+UdsServer::send_to\s*\([^)]+\)\s*\{[^}]+\})', r'\1\n' + method, content)

with open(path, 'w') as f:
    f.write(content)
print(f"Updated {path}")

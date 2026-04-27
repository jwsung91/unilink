import os
import re

wrapper_headers = [
    'unilink/wrapper/tcp_client/tcp_client.hpp',
    'unilink/wrapper/uds_client/uds_client.hpp',
    'unilink/wrapper/serial/serial.hpp',
    'unilink/wrapper/udp/udp.hpp',
]

server_headers = [
    'unilink/wrapper/tcp_server/tcp_server.hpp',
    'unilink/wrapper/uds_server/uds_server.hpp',
    'unilink/wrapper/udp/udp_server.hpp',
]

for path in wrapper_headers:
    with open(path, 'r') as f:
        content = f.read()
    if 'send_blocking' not in content:
        content = re.sub(r'bool\s+send_line\s*\([^)]+\)\s*override;',
                         r'bool send_line(std::string_view line) override;\n  bool send_blocking(std::string_view data) override;\n  bool send_line_blocking(std::string_view line) override;', content)
        with open(path, 'w') as f:
            f.write(content)
        print(f"Updated {path}")

for path in server_headers:
    with open(path, 'r') as f:
        content = f.read()
    if 'send_to_blocking' not in content:
        content = re.sub(r'bool\s+send_to\s*\([^)]+\)\s*override;',
                         r'bool send_to(ClientId client_id, std::string_view data) override;\n  bool send_to_blocking(ClientId client_id, std::string_view data) override;', content)
        with open(path, 'w') as f:
            f.write(content)
        print(f"Updated {path}")

# Now for the implementations
wrapper_impls = [
    'unilink/wrapper/tcp_client/tcp_client.cc',
    'unilink/wrapper/uds_client/uds_client.cc',
    'unilink/wrapper/serial/serial.cc',
    'unilink/wrapper/udp/udp.cc',
]

for path in wrapper_impls:
    with open(path, 'r') as f:
        content = f.read()
    
    changed = False
    if 'std::condition_variable bp_cv_' not in content:
        # Add to Impl or near struct members
        content = re.sub(r'(std::shared_mutex\s+mutex_;)', r'\1\n  std::mutex bp_mutex_;\n  std::condition_variable bp_cv_;', content)
        changed = True

    # Implement send_blocking and send_line_blocking
    if 'send_blocking(' not in content:
        cls = 'TcpClient' if 'tcp_client' in path else ('UdsClient' if 'uds_client' in path else ('Serial' if 'serial' in path else 'Udp'))
        methods = f"""
  bool send_blocking(std::string_view data) {{
    std::unique_lock<std::mutex> lock(bp_mutex_);
    bp_cv_.wait(lock, [this]() {{
      std::shared_lock<std::shared_mutex> rlock(mutex_);
      return !channel_ || !channel_->is_backpressure_active();
    }});
    return send(data);
  }}

  bool send_line_blocking(std::string_view line) {{
    std::unique_lock<std::mutex> lock(bp_mutex_);
    bp_cv_.wait(lock, [this]() {{
      std::shared_lock<std::shared_mutex> rlock(mutex_);
      return !channel_ || !channel_->is_backpressure_active();
    }});
    return send_line(line);
  }}
"""
        # Insert after send_line
        content = re.sub(r'(bool\s+send_line\s*\([^)]+\)\s*\{[^}]+(?:\}[^}]+)*\n\s*\})', r'\1\n' + methods, content, count=1)
        changed = True

    # Hook on_backpressure to notify cv
    if 'bp_cv_.notify_all()' not in content:
        # We find setup_internal_handlers() where channel_->on_backpressure is set.
        # It's usually like:
        # channel_->on_backpressure([this](size_t q) { ...
        #   if (user_on_bp) user_on_bp(q);
        # });
        
        # we will replace the start of lambda `channel_->on_backpressure([this](size_t q) {` with `bp_cv_.notify_all()` 
        content = re.sub(r'(channel_->on_backpressure\([^{]+\{)', r'\1\n      bp_cv_.notify_all();', content)
        changed = True

    if changed:
        with open(path, 'w') as f:
            f.write(content)
        print(f"Updated {path}")

# Server implementations
server_impls = [
    'unilink/wrapper/tcp_server/tcp_server.cc',
    'unilink/wrapper/uds_server/uds_server.cc',
    'unilink/wrapper/udp/udp_server.cc',
]

for path in server_impls:
    with open(path, 'r') as f:
        content = f.read()
    
    changed = False
    if 'std::condition_variable bp_cv_' not in content:
        # Add to Impl
        content = re.sub(r'(std::shared_mutex\s+mutex_;)', r'\1\n  std::mutex bp_mutex_;\n  std::condition_variable bp_cv_;', content)
        changed = True

    # Implement send_to_blocking
    if 'send_to_blocking(' not in content:
        cls = 'TcpServer' if 'tcp_server' in path else ('UdsServer' if 'uds_server' in path else 'UdpServer')
        methods = f"""
  bool send_to_blocking(ClientId client_id, std::string_view data) {{
    std::unique_lock<std::mutex> lock(bp_mutex_);
    bp_cv_.wait(lock, [this, client_id]() {{
      std::shared_lock<std::shared_mutex> rlock(mutex_);
      return !server_ || !server_->is_backpressure_active(client_id);
    }});
    return send_to(client_id, data);
  }}
"""
        content = re.sub(r'(bool\s+send_to\s*\([^)]+\)\s*\{[^}]+(?:\}[^}]+)*\n\s*\})', r'\1\n' + methods, content, count=1)
        changed = True

    if 'bp_cv_.notify_all()' not in content:
        # server setup
        content = re.sub(r'(server_->on_backpressure\([^{]+\{)', r'\1\n      bp_cv_.notify_all();', content)
        changed = True

    if changed:
        with open(path, 'w') as f:
            f.write(content)
        print(f"Updated {path}")

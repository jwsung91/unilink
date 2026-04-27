import re

# Fix tcp_server_session.cc
with open("unilink/transport/tcp_server/tcp_server_session.cc", "r") as f:
    c = f.read()
c = c.replace("if (self->queue_bytes_ + size > self->bp_limit_) return false;", "if (queue_bytes_ + size > bp_limit_) return false;")
c = c.replace("if (self->queue_bytes_ + added > self->bp_limit_) return false;", "if (queue_bytes_ + added > bp_limit_) return false;")
c = re.sub(r'(\n\s*)return;(\n\s*// Use memory pool)', r'\1return false;\2', c) # Fix return; on max buffer size
with open("unilink/transport/tcp_server/tcp_server_session.cc", "w") as f:
    f.write(c)

# Fix serial.cc
with open("unilink/transport/serial/serial.cc", "r") as f:
    c = f.read()
c = c.replace("if (queued_bytes_ + n > bp_limit_) return false;", "if (impl->queued_bytes_ + n > impl->bp_limit_) return false;")
c = c.replace("if (queued_bytes_ + added > bp_limit_) return false;", "if (impl->queued_bytes_ + added > impl->bp_limit_) return false;")
c = c.replace("UNILINK_LOG_ERROR(\"serial\", \"write\", \"Write size exceeds maximum\");\n    return;", "UNILINK_LOG_ERROR(\"serial\", \"write\", \"Write size exceeds maximum\");\n    return false;")
with open("unilink/transport/serial/serial.cc", "w") as f:
    f.write(c)

# Fix udp.cc
with open("unilink/transport/udp/udp.cc", "r") as f:
    c = f.read()
c = c.replace("if (queue_bytes_ + size > bp_limit_) return false;", "if (impl->queue_bytes_ + size > impl->bp_limit_) return false;")
c = c.replace("UNILINK_LOG_ERROR(\"udp\", \"write\", \"Write size exceeds maximum allowed\");\n    return;", "UNILINK_LOG_ERROR(\"udp\", \"write\", \"Write size exceeds maximum allowed\");\n    return false;")
with open("unilink/transport/udp/udp.cc", "w") as f:
    f.write(c)

# Fix udp.hpp
with open("unilink/transport/udp/udp.hpp", "r") as f:
    c = f.read()
c = c.replace("virtual void async_write_to", "virtual bool async_write_to")
with open("unilink/transport/udp/udp.hpp", "w") as f:
    f.write(c)

# Fix tcp_client.cc line 310
with open("unilink/transport/tcp_client/tcp_client.cc", "r") as f:
    c = f.read()
lines = c.split('\n')
for i, l in enumerate(lines):
    if "return;" in l and (300 < i < 320):
        # We need to find the exact line. Let's just fix any `return;` outside lambda.
        pass
with open("unilink/transport/tcp_client/tcp_client.cc", "w") as f:
    f.write(c)

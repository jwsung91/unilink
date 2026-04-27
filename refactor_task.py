import os
import re

files_cc = [
    "unilink/transport/serial/serial.cc",
    "unilink/transport/tcp_client/tcp_client.cc",
    "unilink/transport/tcp_server/tcp_server.cc",
    "unilink/transport/tcp_server/tcp_server_session.cc",
    "unilink/transport/udp/udp.cc",
    "unilink/transport/uds/uds_client.cc",
    "unilink/transport/uds/uds_server.cc",
    "unilink/transport/uds/uds_server_session.cc"
]

files_hpp = [
    "unilink/transport/tcp_server/tcp_server_session.hpp",
    "unilink/transport/uds/uds_server_session.hpp",
    "unilink/transport/tcp_client/tcp_client.hpp", # Check if they have queue_bytes_
    "unilink/transport/uds/uds_client.hpp"
]

def refactor_file(filepath):
    if not os.path.exists(filepath):
        return
    with open(filepath, 'r') as f:
        content = f.read()
    orig = content

    # Replace size_t queue_bytes_ = 0; with std::atomic<size_t> queue_bytes_{0};
    # Both in .cc and .hpp
    content = re.sub(r'\bsize_t\s+queue_bytes_\s*(?:=\s*0|\{0\});', r'std::atomic<size_t> queue_bytes_{0};', content)
    content = re.sub(r'\bsize_t\s+queued_bytes_\s*(?:=\s*0|\{0\});', r'std::atomic<size_t> queued_bytes_{0};', content)
    content = re.sub(r'\bsize_t\s+queue_bytes_\(0\)', r'std::atomic<size_t> queue_bytes_{0}', content)
    content = re.sub(r'\bsize_t\s+queued_bytes_\(0\)', r'std::atomic<size_t> queued_bytes_{0}', content)

    # In constructor initializers: queue_bytes_(0) -> queue_bytes_{0} ? Actually std::atomic can be initialized with (0).
    # But let's let atomic do its job.

    if filepath.endswith('.cc'):
        # Find async_write_* implementations
        method_pattern = re.compile(
            r'^void\s+([A-Za-z0-9_:]+)::async_write_(copy|move|shared|to)\s*\(([^)]+)\)\s*\{',
            re.MULTILINE
        )

        pos = 0
        new_content = ""
        while True:
            match = method_pattern.search(content, pos)
            if not match:
                new_content += content[pos:]
                break

            new_content += content[pos:match.start()]
            new_content += f"bool {match.group(1)}::async_write_{match.group(2)}({match.group(3)}) {{"

            # find body
            brace_count = 1
            i = match.end()
            while i < len(content) and brace_count > 0:
                if content[i] == '{':
                    brace_count += 1
                elif content[i] == '}':
                    brace_count -= 1
                i += 1

            method_body = content[match.end():i-1]

            # We need to change `return;` to `return false;` OUTSIDE of lambdas.
            # Lambdas are typically inside `net::dispatch` or `net::post` or `bind_executor`.
            # We can process line by line, keeping track of curly braces. But a simpler way:
            # We look for the `net::post` or `net::dispatch` call. Everything before it is "outside".
            # If the method doesn't have it (like tcp_server.cc), then all of it is "outside".
            
            post_idx = method_body.find('net::post')
            if post_idx == -1:
                post_idx = method_body.find('net::dispatch')
            
            if post_idx != -1:
                before_post = method_body[:post_idx]
                after_post = method_body[post_idx:]
                
                # Replace return; with return false; in before_post
                before_post = re.sub(r'\breturn\s*;', 'return false;', before_post)

                # Inject the size check right before the post/dispatch
                # Determine which variable is used for size: `n`, `added`, `size`
                # And determine if queue_bytes_ is accessed via `impl->` or `self->` or directly.
                
                qvar = "queue_bytes_"
                if "queued_bytes_" in content:
                    qvar = "queued_bytes_"
                
                impl_var = ""
                if "impl->" + qvar in before_post or "impl_->" + qvar in content:
                    if "impl->" in before_post:
                        impl_var = "impl->"
                    else:
                        impl_var = "impl_->"
                elif "self->" + qvar in before_post or "self->" + qvar in content:
                    impl_var = "self->"
                
                bp_limit_var = impl_var + "bp_limit_"
                q_full_var = impl_var + qvar
                
                size_var = "size"
                if " n =" in before_post or " n=" in before_post:
                    size_var = "n"
                elif " added =" in before_post or " added=" in before_post:
                    size_var = "added"
                elif " size =" in before_post or " size=" in before_post:
                    size_var = "size"
                elif " data->size()" in before_post:
                    size_var = "data->size()"
                elif " data.size()" in before_post:
                    size_var = "data.size()"
                else: # fallback
                    if match.group(2) == 'shared':
                        size_var = "data->size()"
                    else:
                        size_var = "data.size()"
                
                if impl_var:
                    check_code = f"\n  if ({q_full_var} + {size_var} > {bp_limit_var}) return false;\n  "
                else:
                    check_code = f"\n  if ({qvar} + {size_var} > bp_limit_) return false;\n  "

                # special case for udp async_write_to:
                if 'bp_limit_' not in method_body and 'impl->cfg_.enable_memory_pool' in method_body:
                    # UDP uses bp_limit_? udp.cc does use it
                    pass

                # avoid double injection if we already have it
                if "return false;" not in check_code or check_code.strip() not in before_post:
                    before_post = before_post.rstrip() + check_code

                method_body = before_post + after_post
            else:
                # no post/dispatch (e.g. TcpServer)
                # just replace return; with return false;
                # Wait, TcpServer's async_write_copy calls session->async_write_copy(data) which returns bool.
                # If we see session->async_write, we can return it.
                method_body = re.sub(r'\breturn\s*;', 'return false;', method_body)
                method_body = re.sub(r'session->async_write_([a-z]+)\((.*?)\);', r'return session->async_write_\1(\2);', method_body)

            # ensure return true; at the end
            if not method_body.strip().endswith('return true;') and not method_body.strip().endswith('return false;'):
                # Check if last statement is already a return.
                if not re.search(r'return\s+[^;]+;\s*$', method_body.strip()):
                    method_body = method_body.rstrip() + '\n  return true;\n'

            new_content += method_body + '}'
            pos = i

        content = new_content

    if orig != content:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Refactored {filepath}")

for f in files_cc + files_hpp:
    refactor_file(f)

import os
import re

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    orig = content

    # Replace size_t queue_bytes_{0}; or = 0; with atomic
    content = re.sub(r'size_t\s+queue_bytes_\s*(?:=\s*0|\{0\});', r'std::atomic<size_t> queue_bytes_{0};', content)
    content = re.sub(r'size_t\s+queue_bytes_\(0\)', r'std::atomic<size_t> queue_bytes_(0)', content)
    
    # Replace backpressure_active_
    content = re.sub(r'bool\s+backpressure_active_\s*(?:=\s*false|\{false\});', r'std::atomic<bool> backpressure_active_{false};', content)
    content = re.sub(r'bool\s+backpressure_active_\(false\)', r'std::atomic<bool> backpressure_active_(false)', content)

    # In async_write_* implementations, we need to handle the returns.
    # Pattern: bool Class::async_write_copy(memory::ConstByteSpan data) { ... }
    # We will search for all async_write_copy/move/shared methods.
    
    method_pattern = re.compile(
        r'bool\s+([A-Za-z0-9_:]+)::async_write_(copy|move|shared)\s*\(([^)]+)\)\s*\{',
        re.MULTILINE
    )
    
    pos = 0
    new_content = ""
    while True:
        match = method_pattern.search(content, pos)
        if not match:
            new_content += content[pos:]
            break
        
        new_content += content[pos:match.end()]
        
        # Find matching brace
        brace_count = 1
        i = match.end()
        while i < len(content) and brace_count > 0:
            if content[i] == '{':
                brace_count += 1
            elif content[i] == '}':
                brace_count -= 1
            i += 1
            
        method_body = content[match.end():i-1]
        
        # Replace empty return; with return false;
        # But ONLY top-level returns? No, any return inside the method but outside lambdas.
        # But async_write_* uses net::dispatch/post with lambdas!
        # Inside lambdas, `return;` should stay `return;` because lambdas return void.
        # It's safer to only replace `return;` that are NOT inside lambdas.
        
        # Simple heuristic: we replace `return;` with `return false;` ONLY if there's no `[` or `(` on the same line or previous lines opening a lambda. 
        # Actually, let's just split by lines and keep track of lambda depth.
        
        lines = method_body.split('\n')
        lambda_depth = 0
        brace_depth = 0
        for idx, line in enumerate(lines):
            # very rough lambda check
            if 'net::dispatch' in line or 'net::post' in line or 'bind_executor' in line:
                lambda_depth += 1
            if '{' in line:
                brace_depth += line.count('{')
            if '}' in line:
                brace_depth -= line.count('}')
                if brace_depth == 0 and lambda_depth > 0:
                    lambda_depth -= 1
            
            if lambda_depth == 0:
                lines[idx] = re.sub(r'\breturn\s*;', 'return false;', line)
                
        method_body = '\n'.join(lines)
        
        # Append return true; at the end if not there
        if not method_body.strip().endswith('return true;'):
            method_body = method_body.rstrip() + '\n  return true;\n'
            
        new_content += method_body + '}'
        pos = i

    if orig != new_content:
        with open(filepath, 'w') as f:
            f.write(new_content)
        print(f"Refactored methods in {filepath}")

for root, _, files in os.walk('unilink/transport'):
    for file in files:
        if file.endswith('.cc') or file.endswith('.hpp'):
            process_file(os.path.join(root, file))

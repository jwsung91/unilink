import os
import re

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # Change void async_write to bool async_write
    # We want to match: `void async_write_copy(...)`, `virtual void async_write_copy(...)`
    # Also `void async_write_move(...)`, `void async_write_shared(...)`
    new_content = re.sub(r'\bvoid\s+async_write_(copy|move|shared)\b', r'bool async_write_\1', content)

    if new_content != content:
        with open(filepath, 'w') as f:
            f.write(new_content)
        print(f"Modified {filepath}")

for root, _, files in os.walk('unilink'):
    for file in files:
        if file.endswith('.hpp') or file.endswith('.cc'):
            process_file(os.path.join(root, file))

for root, _, files in os.walk('test'):
    for file in files:
        if file.endswith('.hpp') or file.endswith('.cc'):
            process_file(os.path.join(root, file))


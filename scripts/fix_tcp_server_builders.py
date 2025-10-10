#!/usr/bin/env python3
"""
Fix tcp_server() calls to include .unlimited_clients() when missing
"""

import re
import sys
from pathlib import Path

def fix_tcp_server_calls(content):
    """Add .unlimited_clients() to tcp_server() calls that don't have client limit methods"""
    
    # Pattern: tcp_server(...) followed by methods, but not already having client limit setters
    # We look for tcp_server(...).something().build() patterns
    
    lines = content.split('\n')
    result_lines = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        
        # Check if line contains tcp_server
        if 'tcp_server(' in line and '::tcp_server(' in line:
            # Check if already has client limit method
            has_client_limit = any(method in line for method in [
                'unlimited_clients()', 'single_client()', 'multi_client('
            ])
            
            if not has_client_limit:
                # Look ahead to see if client limit is set in next lines
                lookahead = '\n'.join(lines[i:min(i+20, len(lines))])
                has_client_limit_ahead = any(method in lookahead for method in [
                    '.unlimited_clients()', '.single_client()', '.multi_client('
                ])
                
                if not has_client_limit_ahead and '.build()' in lookahead:
                    # Add .unlimited_clients() after tcp_server(...)
                    # Find the closing parenthesis of tcp_server
                    if 'tcp_server(' in line:
                        # Replace pattern: tcp_server(port).method() -> tcp_server(port).unlimited_clients().method()
                        line = re.sub(
                            r'(tcp_server\([^)]+\))(\s*\.\s*)',
                            r'\1.unlimited_clients()\2',
                            line
                        )
        
        result_lines.append(line)
        i += 1
    
    return '\n'.join(result_lines)

def process_file(filepath):
    """Process a single file"""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
        
        new_content = fix_tcp_server_calls(content)
        
        if new_content != content:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            print(f"✓ Fixed: {filepath}")
            return True
        else:
            return False
    except Exception as e:
        print(f"✗ Error processing {filepath}: {e}")
        return False

def main():
    project_root = Path(__file__).parent.parent
    test_dir = project_root / 'test' / 'integration'
    
    print("=== Fixing tcp_server builders ===\n")
    
    fixed_count = 0
    for cc_file in test_dir.rglob('*.cc'):
        if process_file(cc_file):
            fixed_count += 1
    
    print(f"\n=== Done! Fixed {fixed_count} files ===")

if __name__ == '__main__':
    main()


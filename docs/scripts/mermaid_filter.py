import sys
import re

def filter_mermaid(content):
    # 1. ```mermaid 블록을 @mermaid{ ... }로 변환
    def replace_mermaid(match):
        code = match.group(1).strip()
        return f"\n@mermaid{{{code}}}\n"

    mermaid_pattern = re.compile(r"```mermaid\s*(.*?)\s*```", re.DOTALL)
    content = mermaid_pattern.sub(replace_mermaid, content)
    
    # 2. 코드 블록(```cpp 등) 내의 @ 기호를 @@로 변환하여 Doxygen 명령어 오인 방지
    # 마크다운 전체를 대상으로 @ -> @@를 하면 일반 텍스트나 Mermaid 명령어까지 깨지므로,
    # 코드 블록 내부만 타겟팅합니다.
    def escape_at_in_code(match):
        code_content = match.group(0)
        return code_content.replace("@", "@@")

    # 일반 코드 블록 (``` ... ```)
    code_block_pattern = re.compile(r"```.*?```", re.DOTALL)
    content = code_block_pattern.sub(escape_at_in_code, content)
    
    return content

if __name__ == "__main__":
    if len(sys.argv) > 1:
        with open(sys.argv[1], 'r', encoding='utf-8') as f:
            content = f.read()
    else:
        content = sys.stdin.read()
    
    sys.stdout.write(filter_mermaid(content))

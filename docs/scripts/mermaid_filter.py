import sys
import re

def filter_mermaid(content):
    # 1. ```mermaid 블록을 @mermaid{ ... }로 변환
    def replace_mermaid(match):
        code = match.group(1).strip()
        # Ensure newlines are preserved for doxygen verbatim
        # We use a unique marker to avoid interference with other @ commands
        return f"\n\n@mermaid{{{code}}}\n\n"

    # Use a more robust pattern that doesn't over-match
    mermaid_pattern = re.compile(r"```mermaid\s*\n(.*?)\n\s*```", re.DOTALL)
    content = mermaid_pattern.sub(replace_mermaid, content)
    
    # 2. 코드 블록(```cpp 등) 내의 @ 기호를 @@로 변환하여 Doxygen 명령어 오인 방지
    def escape_at_in_code(match):
        code_content = match.group(0)
        return code_content.replace("@", "@@")

    code_block_pattern = re.compile(r"```.*?```", re.DOTALL)
    content = code_block_pattern.sub(escape_at_in_code, content)
    
    # 3. 마크다운 내부 앵커 링크 보정 (Doxygen 경고 방지)
    # [Text](#anchor) -> [Text](@ref anchor) 또는 [Text](#anchor) 형태 유지하되
    # Doxygen이 이해할 수 있도록 도움을 줍니다. 
    # 여기서는 단순히 앵커 형식을 유지하거나, 필요 시 변환 로직을 넣을 수 있습니다.
    # 현재는 Mermaid와 @ 이스케이프가 주 목적이므로 이 부분은 필요시 확장 가능하도록 둡니다.
    
    return content

if __name__ == "__main__":
    if len(sys.argv) > 1:
        with open(sys.argv[1], 'r', encoding='utf-8') as f:
            content = f.read()
    else:
        content = sys.stdin.read()
    
    sys.stdout.write(filter_mermaid(content))

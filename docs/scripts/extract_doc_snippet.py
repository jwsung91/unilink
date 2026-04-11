#!/usr/bin/env python3
"""Extract a compile-checked C++ snippet from a Markdown document.

Markers use the form:
    <!-- doc-compile: snippet_name -->

The next fenced C++ code block after the marker is written to the output path.
"""

from __future__ import annotations

import argparse
import pathlib
import sys


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="Markdown source file")
    parser.add_argument("--snippet", required=True, help="Snippet marker name")
    parser.add_argument("--output", required=True, help="Output .cc file path")
    return parser.parse_args()


def is_cpp_fence(line: str) -> bool:
    stripped = line.strip()
    if not stripped.startswith("```"):
        return False
    info = stripped[3:].strip().lower()
    return info in {"cpp", "c++", "cc"}


def extract_snippet(markdown_path: pathlib.Path, snippet_name: str) -> str:
    lines = markdown_path.read_text(encoding="utf-8").splitlines()
    marker = f"<!-- doc-compile: {snippet_name} -->"

    for index, line in enumerate(lines):
        if line.strip() != marker:
            continue

        cursor = index + 1
        while cursor < len(lines) and not lines[cursor].strip():
            cursor += 1

        if cursor >= len(lines) or not is_cpp_fence(lines[cursor]):
            raise ValueError(
                f"Marker '{snippet_name}' in {markdown_path} is not followed by a fenced C++ block"
            )

        cursor += 1
        snippet_lines: list[str] = []
        while cursor < len(lines) and lines[cursor].strip() != "```":
            snippet_lines.append(lines[cursor])
            cursor += 1

        if cursor >= len(lines):
            raise ValueError(f"Unterminated fenced block for marker '{snippet_name}' in {markdown_path}")

        snippet_text = "\n".join(snippet_lines).strip() + "\n"
        if "int main" not in snippet_text:
            raise ValueError(
                f"Snippet '{snippet_name}' in {markdown_path} does not look like a standalone program"
            )
        return snippet_text

    raise ValueError(f"Marker '{snippet_name}' not found in {markdown_path}")


def main() -> int:
    args = parse_args()
    input_path = pathlib.Path(args.input)
    output_path = pathlib.Path(args.output)

    try:
        snippet = extract_snippet(input_path, args.snippet)
    except Exception as exc:  # pragma: no cover - simple CLI error path
        print(f"error: {exc}", file=sys.stderr)
        return 1

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        f"// Generated from {input_path} [{args.snippet}]\n{snippet}",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

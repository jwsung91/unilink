# unilink Documentation

This directory contains the hand-written documentation for `unilink` plus the configuration and helper scripts used to generate Doxygen output.

## Directory Structure

```text
docs/
├── README.md                 # Documentation maintenance guide
├── index.md                  # Main handwritten documentation index
├── implementation_status.md  # Current codebase snapshot
├── architecture/             # Design and runtime notes
├── config/                   # Doxygen configuration
├── guides/                   # Setup, usage, testing, troubleshooting
├── img/                      # Images referenced by docs
├── reference/                # API guide and reference material
├── scripts/                  # Helper scripts for docs generation/serving
└── tutorials/                # Step-by-step walkthroughs
```

Generated HTML is written to `docs/html/` when Doxygen runs. That output is a generated artefact, not the source of truth.

## Where To Start

- Reader entry point: `docs/index.md`
- Current implementation snapshot: `docs/implementation_status.md`
- Doxygen configuration: `docs/config/Doxyfile`
- Helper scripts: `docs/scripts/generate_docs.sh`, `serve_docs.sh`, `clean_docs.sh`

## Generating Documentation

### Prerequisites

```bash
sudo apt install doxygen graphviz
```

### Supported Methods

Using the helper script:

```bash
./docs/scripts/generate_docs.sh
```

Using the top-level Makefile:

```bash
make docs
```

Using the docs Makefile:

```bash
cd docs
make generate
```

Running Doxygen directly:

```bash
doxygen docs/config/Doxyfile
```

If you invoke Doxygen directly, set `PROJECT_NUMBER` yourself if you want the generated version banner to match `project(VERSION ...)` from `CMakeLists.txt`.

## Viewing Generated HTML

```bash
./docs/scripts/serve_docs.sh
```

Or:

```bash
cd docs/html
python3 -m http.server 8000
```

## Maintenance Notes

- Keep links aligned with real file names and directory layout.
- Prefer documenting verified behavior over aspirational behavior.
- Keep public wrapper/builder APIs and transport-internal behavior in separate documents.
- When documenting callbacks or methods, verify signatures against headers under `unilink/wrapper/` and `unilink/builder/`.
- Treat runnable examples under `examples/` as the preferred source for tutorial snippets when possible.
- When build flags or defaults change, update `README.md`, `docs/index.md`, and `docs/guides/setup/build_guide.md` together.
- When adding public APIs, update `docs/reference/api_guide.md` and `docs/implementation_status.md`.
- When changing config behavior, update both `docs/reference/api_guide.md` and architecture notes that mention configuration flow.
- Keep document roles separated to avoid copy drift:
  - `quickstart`: shortest successful path
  - `tutorials`: step-by-step workflows
  - `reference/api_guide.md`: protocol and API surface details
  - `examples/*/README.md`: runnable example commands and CLI usage

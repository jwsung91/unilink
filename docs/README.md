# unilink Documentation

This directory contains all documentation-related files for the unilink library.

## Directory Structure

```
docs/
├── README.md              # This file
├── api_guide.md           # API usage guide
├── config/                # Documentation configuration
│   └── Doxyfile          # Doxygen configuration
├── scripts/               # Documentation generation scripts
│   ├── generate_docs.sh  # Generate documentation
│   ├── serve_docs.sh     # Serve documentation locally
│   └── clean_docs.sh     # Clean generated files
└── html/                  # Generated HTML documentation (after generation)
```

## Generating Documentation

### Prerequisites

Install Doxygen and Graphviz:

```bash
# Ubuntu/Debian
sudo apt install doxygen graphviz

# CentOS/RHEL/Fedora
sudo yum install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows
# Download from https://www.doxygen.nl/download.html
```

### Generation Methods

#### Method 1: Using the Script
```bash
./docs/scripts/generate_docs.sh
```

#### Method 2: Using CMake
```bash
mkdir build && cd build
cmake .. -DBUILD_DOCUMENTATION=ON
make docs
```

#### Method 3: Using Makefile (from project root)
```bash
make docs
```

#### Method 4: Using Documentation Makefile (from docs directory)
```bash
cd docs
make generate
```

#### Method 5: Direct Doxygen
```bash
doxygen docs/config/Doxyfile
```

### Viewing Documentation

After generation, you can view the documentation in several ways:

#### Method 1: Direct File Access
```bash
# Open in browser
xdg-open docs/html/index.html  # Linux
open docs/html/index.html      # macOS
start docs/html/index.html     # Windows
```

#### Method 2: Local Server (Recommended)
```bash
# Using the provided script
make serve-docs
# or
./docs/scripts/serve_docs.sh

# Manual server
cd docs/html && python3 -m http.server 8000
# Then open http://localhost:8000
```

### Additional Scripts

#### Clean Documentation
```bash
make clean-docs
# or
./docs/scripts/clean_docs.sh
```

#### Serve Documentation
```bash
make serve-docs
# or
./docs/scripts/serve_docs.sh
```

## Documentation Structure

The generated documentation includes:

- **Main Page**: Overview of the library and quick start guide
- **Classes**: Detailed API documentation for all classes
- **Files**: Source file documentation
- **Namespaces**: Namespace organization
- **Examples**: Code examples and usage patterns
- **Modules**: Logical grouping of related functionality

## Features

- **Cross-references**: Links between related classes and functions
- **Call graphs**: Visual representation of function call relationships
- **Inheritance diagrams**: Class hierarchy visualization
- **Collaboration diagrams**: Class interaction visualization
- **Search functionality**: Full-text search across all documentation
- **Code examples**: Inline code examples with syntax highlighting

## Customization

The documentation can be customized by modifying the `Doxyfile` configuration:

- **HTML output**: Custom CSS styling, navigation, and layout
- **Input sources**: Additional source files and directories
- **Output format**: HTML, LaTeX, RTF, XML, etc.
- **Diagrams**: Graphviz integration for class and call graphs
- **Search**: Full-text search configuration

## Troubleshooting

### Common Issues

1. **Doxygen not found**: Install Doxygen using the package manager
2. **Graphviz not found**: Install Graphviz for diagram generation
3. **Permission denied**: Ensure write permissions for the docs directory
4. **Missing diagrams**: Check Graphviz installation and PATH

### Debug Information

Enable verbose output:
```bash
doxygen -d Doxyfile
```

Check Doxygen version:
```bash
doxygen --version
```

## Continuous Integration

The documentation can be automatically generated in CI/CD pipelines:

```yaml
# GitHub Actions example
- name: Generate Documentation
  run: |
    sudo apt install doxygen graphviz
    make docs
    # Upload docs to GitHub Pages or artifact storage
```

## Contributing

When adding new classes or functions, please include proper Doxygen comments:

```cpp
/**
 * @brief Brief description of the function
 * @param param1 Description of parameter 1
 * @param param2 Description of parameter 2
 * @return Description of return value
 * @throws ExceptionType When this exception is thrown
 * @note Additional notes
 * @warning Important warnings
 * @example
 * @code
 * // Example usage
 * @endcode
 */
```

# Documentation Makefile for unilink library
# This Makefile contains only documentation-related targets

.PHONY: docs generate cmake serve clean help

# Default target
docs: generate

# Generate documentation
generate:
	@echo "📚 Generating documentation..."
	cd .. && ./docs/scripts/generate_docs.sh

# Generate documentation with CMake
cmake:
	@echo "📚 Generating documentation with CMake..."
	mkdir -p ../build
	cd ../build && cmake .. -DBUILD_DOCUMENTATION=ON
	cd ../build && make docs

# Serve documentation locally
serve:
	@echo "🌐 Serving documentation..."
	cd .. && ./docs/scripts/serve_docs.sh

# Clean documentation
clean:
	@echo "🧹 Cleaning documentation..."
	cd .. && ./docs/scripts/clean_docs.sh

# Show help
help:
	@echo "unilink Documentation Makefile"
	@echo "=============================="
	@echo ""
	@echo "Available targets:"
	@echo "  generate     - Generate documentation (default)"
	@echo "  cmake        - Generate documentation with CMake"
	@echo "  serve        - Serve documentation locally"
	@echo "  clean        - Clean documentation files"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make generate serve    # Generate and serve docs"
	@echo "  make clean generate    # Clean and regenerate"
	@echo "  make cmake             # Use CMake for generation"
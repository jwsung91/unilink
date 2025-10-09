# Multi-stage build for optimized production image
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    libboost-dev \
    libboost-system-dev \
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Configure and build
RUN rm -rf build && mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DUNILINK_ENABLE_CONFIG=ON \
        -DBUILD_EXAMPLES=ON \
        -DBUILD_TESTING=ON && \
    cmake --build . -j $(nproc)

# Run tests
RUN cd build && ctest --output-on-failure

# Generate documentation
RUN cd build && make docs

# Production stage
FROM ubuntu:22.04 AS production

# Install only runtime dependencies
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1000 appuser

# Set working directory
WORKDIR /app

# Copy built artifacts from builder stage
COPY --from=builder /app/build/lib /app/lib
COPY --from=builder /app/build/examples /app/examples
COPY --from=builder /app/build/docs/html /app/docs

# Copy source headers for development
COPY --from=builder /app/unilink /app/unilink
COPY --from=builder /app/CMakeLists.txt /app/

# Change ownership to non-root user
RUN chown -R appuser:appuser /app

# Switch to non-root user
USER appuser

# Default command
CMD ["./examples/interface_socket_example"]

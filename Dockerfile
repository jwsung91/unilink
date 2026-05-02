# Multi-stage build for optimized production image
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    software-properties-common \
    wget \
    && (wget -O /etc/apt/trusted.gpg.d/ubuntu-toolchain-r.gpg "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x1E9377A2BA9EF27F" || true) && \
    echo "deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu jammy main" > /etc/apt/sources.list.d/ubuntu-toolchain-r-test.list && \
    apt-get update && apt-get install -y \
    gcc-13 \
    g++-13 \
    libboost-dev \
    libboost-system-dev \
    python3 \
    doxygen \
    graphviz \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Configure and build
RUN rm -rf build && mkdir build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=gcc-13 \
        -DCMAKE_CXX_COMPILER=g++-13 \
        -DCMAKE_CXX_STANDARD=20 \
        -DUNILINK_ENABLE_CONFIG=ON \
        -DUNILINK_ENABLE_INSTALL=ON \
        -DUNILINK_ENABLE_PKGCONFIG=ON \
        -DUNILINK_ENABLE_EXPORT_HEADER=ON \
        -DUNILINK_BUILD_EXAMPLES=ON \
        -DUNILINK_BUILD_TESTS=OFF \
        -DUNILINK_BUILD_DOCS=OFF && \
    cmake --build . -j $(nproc)

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
COPY --from=builder /app/build/lib/libunilink_static.a /app/lib/libunilink.a
COPY --from=builder /app/build/lib/libunilink.so* /app/lib/
COPY --from=builder /app/build/bin /app/bin

# Create docs directory (documentation is optional)
RUN mkdir -p /app/docs

# Copy source headers for development
COPY --from=builder /app/unilink /app/unilink
COPY --from=builder /app/CMakeLists.txt /app/

# Change ownership to non-root user
RUN chown -R appuser:appuser /app

# Switch to non-root user
USER appuser

# Default command
CMD ["/app/bin/echo_tcp_server"]

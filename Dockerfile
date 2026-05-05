# Multi-stage build for optimized production image
FROM --platform=$BUILDPLATFORM ubuntu:24.04 AS builder

ARG TARGETARCH
ENV VCPKG_FORCE_SYSTEM_BINARIES=1

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    git \
    ninja-build \
    pkg-config \
    tar \
    unzip \
    wget \
    zip \
    gcc-13 \
    g++-13 \
    python3 \
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Map Docker arch to vcpkg triplet
RUN if [ "$TARGETARCH" = "amd64" ]; then \
        echo "x64-linux-dynamic" > /tmp/triplet; \
    elif [ "$TARGETARCH" = "arm64" ]; then \
        echo "arm64-linux-dynamic" > /tmp/triplet; \
    else \
        echo "x64-linux-dynamic" > /tmp/triplet; \
    fi

# Install third-party C++ dependencies. Boost version policy is enforced by
# CMake; vcpkg is only the dependency supplier.
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git /opt/vcpkg && \
    /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics && \
    TRIPLET=$(cat /tmp/triplet) && \
    /opt/vcpkg/vcpkg install boost-asio boost-system spdlog --triplet $TRIPLET --clean-after-build

# Configure and build
RUN TRIPLET=$(cat /tmp/triplet) && \
    rm -rf build && mkdir build && cd build && \
    cmake .. \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=gcc-13 \
        -DCMAKE_CXX_COMPILER=g++-13 \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_TARGET_TRIPLET=$TRIPLET \
        -DUNILINK_ENABLE_CONFIG=ON \
        -DUNILINK_ENABLE_INSTALL=ON \
        -DUNILINK_ENABLE_PKGCONFIG=ON \
        -DUNILINK_ENABLE_EXPORT_HEADER=ON \
        -DUNILINK_BUILD_EXAMPLES=ON \
        -DUNILINK_BUILD_TESTS=OFF \
        -DUNILINK_BUILD_DOCS=OFF && \
    cmake --build . -j $(nproc)

# Production stage
FROM ubuntu:24.04 AS production

RUN apt-get update && apt-get install -y libstdc++6 && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN useradd -m -u 1001 appuser

# Set working directory
WORKDIR /app

# Copy built artifacts from builder stage
COPY --from=builder /app/build/lib/libunilink_static.a /app/lib/libunilink.a
COPY --from=builder /app/build/lib/libunilink.so* /app/lib/
COPY --from=builder /app/build/bin /app/bin
COPY --from=builder /app/build-test/vcpkg_installed/*/lib/*.so* /app/lib/ 2>/dev/null || \
    COPY --from=builder /opt/vcpkg/installed/*/lib/*.so* /app/lib/

ENV LD_LIBRARY_PATH=/app/lib

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

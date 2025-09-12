FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    libboost-dev libboost-system-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN rm -rf build && mkdir build && cd build && cmake .. && cmake --build .

CMD ["./build/interface_socket"]

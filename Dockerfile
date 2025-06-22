# Use Ubuntu as base image
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    librdkafka-dev \
    nlohmann-json3-dev \
    python3 \
    python3-pip \
    python3-dev \
    pybind11-dev \
    pkg-config \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build C++ components
WORKDIR /app/backend
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Install Python dependencies if requirements.txt exists
WORKDIR /app
RUN if [ -f requirements.txt ]; then pip3 install -r requirements.txt; fi

# Expose any ports your application might use
EXPOSE 8000

# Set the default command
CMD ["./backend/build/order_book"]
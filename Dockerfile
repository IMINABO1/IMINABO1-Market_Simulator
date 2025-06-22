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
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build C++ components in backend
WORKDIR /app/backend
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Build C++ components in backend_py (if CMakeLists.txt exists)
WORKDIR /app/backend_py
RUN if [ -f CMakeLists.txt ]; then \
        mkdir -p build && cd build && \
        cmake .. && \
        make -j$(nproc); \
    fi

# Install Python dependencies
WORKDIR /app
RUN if [ -f requirements.txt ]; then pip3 install -r requirements.txt; fi

# Install Python packages from requirements.txt in backend_py
COPY backend_py/requirements.txt /tmp/requirements.txt
RUN pip3 install -r /tmp/requirements.txt

# Create necessary directories
RUN mkdir -p data logs

# Expose ports for gRPC server, FastAPI, and order book
EXPOSE 8000 50051

# Create startup script
RUN echo '#!/bin/bash\n\
# Start the order book gRPC server in background\n\
./backend/build/order_book &\n\
\n\
# Wait a moment for gRPC server to initialize\n\
sleep 5\n\
\n\
# Start Python data collection scripts in background (if they exist)\n\
if [ -f "data_collector.py" ]; then python3 data_collector.py & fi\n\
if [ -f "signal_generator.py" ]; then python3 signal_generator.py & fi\n\
if [ -f "portfolio_manager.py" ]; then python3 portfolio_manager.py & fi\n\
\n\
# Start FastAPI application\n\
cd /app && python3 -m uvicorn main:app --host 0.0.0.0 --port 8000\n\
' > /app/start.sh && chmod +x /app/start.sh

# Set the default command to run the startup script
CMD ["/app/start.sh"]
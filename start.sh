#!/bin/bash

# Function to handle cleanup on exit
cleanup() {
    echo "Shutting down gracefully..."
    pkill -P $$
    exit 0
}

# Set up signal handlers
trap cleanup SIGTERM SIGINT

echo "Starting Trading System with FastAPI..."

# Wait for Kafka to be ready (more robust check)
echo "Waiting for Kafka to be ready..."
for i in {1..30}; do
    if timeout 5 bash -c "</dev/tcp/kafka/29092" 2>/dev/null; then
        echo "Kafka is ready!"
        break
    else
        echo "Kafka not ready, waiting... (attempt $i/30)"
        sleep 2
    fi
done

# Double check with a longer wait
sleep 10

# Start the C++ order book (gRPC server) in background
echo "Starting Order Book gRPC Server..."
if [ -f "./backend/build/order_book" ]; then
    ./backend/build/order_book &
    ORDER_BOOK_PID=$!
    echo "Order Book gRPC Server started with PID: $ORDER_BOOK_PID"
else
    echo "Warning: Order book executable not found"
fi

# Wait for gRPC server to initialize
sleep 5

# Start Python data collection scripts in background
echo "Starting Python trading scripts..."

if [ -f "data_collector.py" ]; then
    python3 data_collector.py &
    DATA_COLLECTOR_PID=$!
    echo "Data Collector started with PID: $DATA_COLLECTOR_PID"
fi

if [ -f "signal_generator.py" ]; then
    python3 signal_generator.py &
    SIGNAL_GENERATOR_PID=$!
    echo "Signal Generator started with PID: $SIGNAL_GENERATOR_PID"
fi

if [ -f "portfolio_manager.py" ]; then
    python3 portfolio_manager.py &
    PORTFOLIO_MANAGER_PID=$!
    echo "Portfolio Manager started with PID: $PORTFOLIO_MANAGER_PID"
fi

# Wait a bit for all services to initialize
sleep 5

# Start FastAPI application (foreground process)
echo "Starting FastAPI Web Interface..."
if [ -f "main.py" ]; then
    python3 -m uvicorn main:app --host 0.0.0.0 --port 8000
else
    echo "Error: main.py not found"
    echo "Available Python files:"
    ls -la *.py
    exit 1
fi
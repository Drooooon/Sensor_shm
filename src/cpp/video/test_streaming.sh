#!/bin/bash

# Test script for producer-consumer video streaming

echo "=== Video Streaming Producer-Consumer Test ==="
echo ""

# Function to cleanup background processes
cleanup() {
    echo ""
    echo "Cleaning up processes..."
    if [ ! -z "$PRODUCER_PID" ]; then
        kill $PRODUCER_PID 2>/dev/null
        echo "Stopped producer process"
    fi
    if [ ! -z "$CONSUMER_PID" ]; then
        kill $CONSUMER_PID 2>/dev/null
        echo "Stopped consumer process"
    fi
    
    # Clean up shared memory
    echo "Cleaning up shared memory..."
    rm -f /dev/shm/yuyv_shm* 2>/dev/null
    
    echo "Cleanup completed"
    exit 0
}

# Set up signal handlers
trap cleanup SIGINT SIGTERM

# Check if executables exist
if [ ! -f "./producer_process" ]; then
    echo "Error: producer_process executable not found. Run 'make producer_process' first."
    exit 1
fi

if [ ! -f "./consumer_process" ]; then
    echo "Error: consumer_process executable not found. Run 'make consumer_process' first."
    exit 1
fi

# Clean up any existing shared memory
echo "Cleaning up any existing shared memory..."
rm -f /dev/shm/yuyv_shm* 2>/dev/null

echo "Starting test..."
echo ""

# Option 1: Run both processes automatically
echo "Choose test mode:"
echo "1) Auto mode - Start both producer and consumer automatically"
echo "2) Manual mode - Instructions for running in separate terminals"
echo ""
read -p "Enter choice (1 or 2): " choice

case $choice in
    1)
        echo "Starting auto mode..."
        echo ""
        
        # Start producer in background
        echo "Starting producer process..."
        ./producer_process &
        PRODUCER_PID=$!
        echo "Producer started with PID: $PRODUCER_PID"
        
        # Wait a moment for producer to initialize
        sleep 2
        
        # Start consumer in foreground
        echo "Starting consumer process..."
        echo "Press 'q' or ESC in the video window to stop both processes"
        ./consumer_process
        
        # Consumer finished, cleanup
        cleanup
        ;;
        
    2)
        echo "Manual mode instructions:"
        echo ""
        echo "1. Open a second terminal and navigate to: $(pwd)"
        echo ""
        echo "2. In the first terminal (this one), run:"
        echo "   ./producer_process"
        echo ""
        echo "3. In the second terminal, run:"
        echo "   ./consumer_process"
        echo ""
        echo "4. Press 'q' or ESC in the video window to stop the consumer"
        echo "5. Press Ctrl+C in the producer terminal to stop the producer"
        echo ""
        echo "Note: Start the producer first, then the consumer."
        echo ""
        echo "Press Enter to continue..."
        read
        ;;
        
    *)
        echo "Invalid choice. Exiting."
        exit 1
        ;;
esac

#!/bin/bash

# Test script for producer-consumer video streaming
# Updated for the new build directory structure

echo "🎥 Video Streaming Producer-Consumer Test"
echo "========================================"
echo ""

# Get the script directory and navigate to parent (video) directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VIDEO_DIR="$(dirname "$SCRIPT_DIR")"
cd "$VIDEO_DIR"

echo "Working directory: $(pwd)"
echo ""

# --- 关键修复: 定义可执行文件的正确路径 ---
BIN_DIR="./build/bin"
PRODUCER_EXEC="$BIN_DIR/producer_process"
CONSUMER_FILES_EXEC="$BIN_DIR/consumer_process"
CONSUMER_GUI_EXEC="$BIN_DIR/consumer_gui"

# Function to cleanup background processes
cleanup() {
    echo ""
    echo "🧹 Cleaning up processes..."
    # Use pkill to robustly stop the producer process by name
    pkill -f "$PRODUCER_EXEC" 2>/dev/null
    echo "✅ Stopped producer process"
    
    echo "🗑️  Cleaning up shared memory..."
    rm -f /dev/shm/yuyv_shm* 2>/dev/null
    
    echo "✅ Cleanup completed"
    exit 0
}

# Set up signal handlers
trap cleanup SIGINT SIGTERM

# Check if executables exist using the correct paths
if [ ! -f "$PRODUCER_EXEC" ]; then
    echo "❌ Error: $PRODUCER_EXEC not found."
    echo "   Run 'make all' first."
    exit 1
fi
if [ ! -f "$CONSUMER_FILES_EXEC" ]; then
    echo "❌ Error: $CONSUMER_FILES_EXEC not found."
    echo "   Run 'make all' first."
    exit 1
fi
if [ ! -f "$CONSUMER_GUI_EXEC" ]; then
    echo "❌ Error: $CONSUMER_GUI_EXEC not found."
    echo "   Run 'make all' first."
    exit 1
fi

# Clean up any existing shared memory
echo "🧹 Cleaning up any existing shared memory..."
rm -f /dev/shm/yuyv_shm* 2>/dev/null

echo "🚀 Starting test..."
echo ""

# Test mode selection
echo "Choose test mode:"
echo "  1) 🤖 Auto mode - Start both processes automatically"
echo "  2) 👥 Manual mode - Instructions for running in separate terminals"
echo ""
read -p "Enter choice (1 or 2): " choice

# --- Consumer selection logic ---
CONSUMER_TO_RUN="$CONSUMER_GUI_EXEC" # Default to GUI version
echo ""
echo "Choose consumer type:"
echo "  1) 🖼️  GUI Consumer (displays video stream)"
echo "  2) 💾 File-Saving Consumer (saves frames, no window)"
read -p "Enter choice (1 or 2, default is 1): " consumer_choice

if [[ "$consumer_choice" == "2" ]]; then
    CONSUMER_TO_RUN="$CONSUMER_FILES_EXEC"
fi
echo "Selected consumer: $CONSUMER_TO_RUN"
echo ""


case $choice in
    1)
        echo "🤖 Starting automatic mode..."
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        
        # Start producer in background using the correct path
        echo "📹 Starting producer process..."
        $PRODUCER_EXEC &
        PRODUCER_PID=$!
        echo "✅ Producer started with PID: $PRODUCER_PID"
        
        echo "⏳ Waiting for producer to initialize..."
        sleep 3
        
        if ! ps -p $PRODUCER_PID > /dev/null; then
            echo "❌ Error: Producer process failed to start or exited prematurely."
            cleanup
        fi

        # Start selected consumer in foreground using the correct path
        echo "🖥️  Starting consumer process: $CONSUMER_TO_RUN"
        echo "📝 Controls: Press 'q' or ESC in the video window to stop"
        echo ""
        $CONSUMER_TO_RUN
        
        echo "Consumer finished. Cleaning up..."
        cleanup
        ;;
        
    2)
        echo "👥 Manual mode instructions:"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "📋 Step-by-step instructions:"
        echo ""
        echo "1. 🖥️  Open a second terminal and navigate to this directory:"
        echo "   cd $(pwd)"
        echo ""
        echo "2. 📹 In the first terminal (this one), run the producer:"
        echo "   $PRODUCER_EXEC"
        echo ""
        echo "3. 🖥️  In the second terminal, run your chosen consumer:"
        echo "   For GUI display: $CONSUMER_GUI_EXEC"
        echo "   For saving files: $CONSUMER_FILES_EXEC"
        echo ""
        echo "4. 🎮 Controls:"
        echo "   - Press 'q' or ESC in the video window to stop the GUI consumer"
        echo "   - The file-saving consumer will stop automatically"
        echo "   - Press Ctrl+C in the producer terminal to stop it"
        echo ""
        ;;
        
    *)
        echo "❌ Invalid choice. Exiting."
        exit 1
        ;;
esac
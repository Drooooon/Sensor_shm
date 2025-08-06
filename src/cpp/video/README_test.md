# Video Streaming Test - Producer/Consumer Setup

This test demonstrates a real-time video streaming system using shared memory for inter-process communication.

## Overview

- **Producer Process**: Captures YUYV video frames from camera and writes them to shared memory
- **Consumer Process**: Reads YUYV frames from shared memory and displays them using OpenCV

## Files

- `producer_process.cpp` - Video capture and shared memory writer
- `consumer_process.cpp` - Shared memory reader and video display
- `test_streaming.sh` - Test script to run both processes
- `producer_process` - Compiled producer executable
- `consumer_process` - Compiled consumer executable

## Usage

### Option 1: Automatic Test (Recommended)
```bash
./test_streaming.sh
```
Choose option 1 for automatic mode. This will:
1. Start the producer process in the background
2. Start the consumer process in the foreground
3. Display the video stream in an OpenCV window
4. Press 'q' or ESC to stop both processes

### Option 2: Manual Test
```bash
# Terminal 1 - Start producer
./producer_process

# Terminal 2 - Start consumer (after producer is running)
./consumer_process
```

## Requirements

- Camera connected (defaults to /dev/video0)
- OpenCV with highgui support for display
- Sufficient shared memory space

## Technical Details

- **Video Format**: YUYV 422 (1920x1080)
- **Shared Memory**: 20MB total, 5MB buffer size
- **Frame Rate**: ~30 FPS (producer has 33ms delay)
- **Display**: Resized to 960x540 for better screen fit

## Controls

- **Consumer Window**: Press 'q' or ESC to quit
- **Producer**: Ctrl+C to stop (if running manually)

## Troubleshooting

1. **Camera not found**: Make sure camera is connected and accessible
2. **Shared memory issues**: Clean up with `rm -f /dev/shm/yuyv_shm*`
3. **Display issues**: Ensure X11 forwarding is enabled if using SSH
4. **Permission issues**: Check camera device permissions

## Build

```bash
make producer_process consumer_process
# or
make test
```

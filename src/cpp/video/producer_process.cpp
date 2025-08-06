#include "image_shm_manager.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <chrono>
#include <thread>

int main() {
  // Initialize shared memory for YUYV data
  // YUYV 1920x1080 = ~4MB per frame, need space for 3 buffers + overhead
  ImageShmManager yuyv_shm("yuyv_shm");
  if (yuyv_shm.create_and_init(20 * 1024 * 1024, 5 * 1024 * 1024) !=
          ShmStatus::Success) {
    std::cerr << "Failed to initialize shared memory" << std::endl;
    return 1;
  }

  // Initialize camera with better error handling
  std::cout << "Attempting to open camera device 0..." << std::endl;
  cv::VideoCapture cap;
  
  // Try opening camera with V4L2 backend first
  cap.open(0, cv::CAP_V4L2);
  if (!cap.isOpened()) {
    std::cout << "V4L2 backend failed, trying default backend..." << std::endl;
    cap.open(0);
  }
  
  if (!cap.isOpened()) {
    std::cerr << "Failed to open camera with any backend" << std::endl;
    return 1;
  }
  std::cout << "Camera opened successfully!" << std::endl;
  
  std::cout << "Setting camera properties..." << std::endl;
  
  // Set buffer size to 1 to reduce latency
  cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
  
  // Set frame format and size
  cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
  
  // Set FPS
  cap.set(cv::CAP_PROP_FPS, 30);
  
  // Try to set YUYV format (may not work on all cameras)
  //cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));

  // Check actual camera settings
  int actual_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
  int actual_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
  double actual_fps = cap.get(cv::CAP_PROP_FPS);
  int buffer_size = cap.get(cv::CAP_PROP_BUFFERSIZE);
  
  std::cout << "Camera settings - Width: " << actual_width 
            << ", Height: " << actual_height 
            << ", FPS: " << actual_fps
            << ", Buffer size: " << buffer_size << std::endl;

  uint64_t frame_version = 0;
  cv::Mat yuyv_frame;

  std::cout << "Starting video capture producer..." << std::endl;
  std::cout << "Press Ctrl+C to stop" << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  
  // Warm up camera - grab a few frames to stabilize
  std::cout << "Warming up camera..." << std::endl;
  cv::Mat warm_frame;
  for (int i = 0; i < 5; i++) {
    cap >> warm_frame;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  
  std::cout << "Attempting to read first frame..." << std::endl;

  // More robust frame reading with non-blocking approach
  bool first_frame_captured = false;
  
  for (int attempt = 0; attempt < 10 && !first_frame_captured; attempt++) {
    std::cout << "Frame read attempt " << (attempt + 1) << "/10" << std::endl;
    
    // Use grab() and retrieve() for better control
    if (cap.grab()) {
      if (cap.retrieve(yuyv_frame)) {
        if (!yuyv_frame.empty()) {
          std::cout << "First frame captured successfully!" << std::endl;
          first_frame_captured = true;
          break;
        }
      }
    }
    
    std::cout << "Attempt failed, waiting 200ms..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  
  if (!first_frame_captured) {
    std::cerr << "Failed to capture first frame after 10 attempts" << std::endl;
    return 1;
  }

  // Main capture loop with better error handling
  std::cout << "Starting main capture loop..." << std::endl;
  
  while (true) {
    // Use grab() and retrieve() for non-blocking frame capture
    if (!cap.grab()) {
      std::cerr << "Failed to grab frame, retrying..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    
    if (!cap.retrieve(yuyv_frame)) {
      std::cerr << "Failed to retrieve frame, retrying..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    
    if (yuyv_frame.empty()) {
      std::cerr << "Captured empty frame, retrying..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    // Debug: Print frame information on first frame
    if (frame_version == 0) {
      size_t frame_data_size = yuyv_frame.total() * yuyv_frame.elemSize();
      std::cout << "Frame info - Cols: " << yuyv_frame.cols 
                << ", Rows: " << yuyv_frame.rows 
                << ", Channels: " << yuyv_frame.channels()
                << ", ElemSize: " << yuyv_frame.elemSize() 
                << ", Total: " << yuyv_frame.total()
                << ", Data size: " << frame_data_size << " bytes (" 
                << frame_data_size / (1024.0 * 1024.0) << " MB)" << std::endl;
      std::cout << "Buffer size configured: " << (5 * 1024 * 1024) << " bytes (5 MB)" << std::endl;
    }

    // Write YUYV to shared memory
    if (yuyv_shm.write_image(yuyv_frame.data,
                             yuyv_frame.total() * yuyv_frame.elemSize(),
                             yuyv_frame.cols, yuyv_frame.rows, 2, frame_version,
                             ImageFormat::YUYV) != ShmStatus::Success) {
      std::cerr << "Failed to write YUYV to shared memory" << std::endl;
      continue;
    }

    frame_version++;

    // Print status every 30 frames
    if (frame_version % 30 == 0) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
      std::cout << "Producer: Frame " << frame_version << " written (elapsed: " << elapsed << "s)" << std::endl;
    }

    // Add a small delay to prevent overwhelming the system
    std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
  }

  std::cout << "Cleaning up producer resources..." << std::endl;
  cap.release();
  yuyv_shm.unmap_and_close();
  yuyv_shm.unlink_shm();

  std::cout << "Producer completed successfully." << std::endl;
  return 0;
}

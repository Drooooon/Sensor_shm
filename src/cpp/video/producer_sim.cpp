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

  // Create a simulated YUYV frame (1920x1080x2 bytes)
  int width = 1920;
  int height = 1080;
  cv::Mat yuyv_frame(height, width, CV_8UC2);
  
  // Fill with some test pattern data
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t* pixel = yuyv_frame.ptr<uint8_t>(y, x * 2);
      pixel[0] = (x + y) % 256; // Y component
      pixel[1] = (x * 2) % 256; // UV component  
    }
  }

  uint64_t frame_version = 0;

  std::cout << "Starting simulated video producer..." << std::endl;
  std::cout << "Frame info - Cols: " << yuyv_frame.cols 
            << ", Rows: " << yuyv_frame.rows 
            << ", Channels: " << yuyv_frame.channels()
            << ", ElemSize: " << yuyv_frame.elemSize() 
            << ", Total: " << yuyv_frame.total() << std::endl;
            
  size_t frame_data_size = yuyv_frame.total() * yuyv_frame.elemSize();
  std::cout << "Frame data size: " << frame_data_size << " bytes (" 
            << frame_data_size / (1024.0 * 1024.0) << " MB)" << std::endl;
  std::cout << "Buffer size configured: " << (5 * 1024 * 1024) << " bytes (5 MB)" << std::endl;
  
  if (frame_data_size > (5 * 1024 * 1024)) {
    std::cerr << "ERROR: Frame size (" << frame_data_size << ") > Buffer size (" << (5 * 1024 * 1024) << ")" << std::endl;
    return 1;
  }

  std::cout << "Press Ctrl+C to stop" << std::endl;

  auto start_time = std::chrono::steady_clock::now();

  for (int i = 0; i < 100; i++) { // Write 100 test frames
    // Write YUYV to shared memory
    if (yuyv_shm.write_image(yuyv_frame.data,
                             frame_data_size,
                             yuyv_frame.cols, yuyv_frame.rows, 2, frame_version,
                             ImageFormat::YUYV) != ShmStatus::Success) {
      std::cerr << "Failed to write YUYV to shared memory at frame " << frame_version << std::endl;
      break;
    }

    frame_version++;

    // Print status every 30 frames
    if (frame_version % 30 == 0) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
      std::cout << "Producer: Frame " << frame_version << " written (elapsed: " << elapsed << "s)" << std::endl;
    }

    // Add a small delay to simulate real capture
    std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
  }

  std::cout << "Cleaning up producer resources..." << std::endl;
  yuyv_shm.unmap_and_close();
  yuyv_shm.unlink_shm();

  std::cout << "Simulated producer completed successfully. Total frames: " << frame_version << std::endl;
  return 0;
}

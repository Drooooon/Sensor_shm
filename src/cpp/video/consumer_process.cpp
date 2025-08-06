#include "image_shm_manager.h"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <chrono>
#include <thread>

int main() {
  // Initialize shared memory for reading YUYV data
  ImageShmManager yuyv_shm("yuyv_shm");
  
  // Try to connect to existing shared memory (created by producer)
  std::cout << "Consumer: Waiting for producer to create shared memory..." << std::endl;
  
  // Wait for shared memory to be available
  int retry_count = 0;
  const int max_retries = 30; // Wait up to 30 seconds
  
  while (retry_count < max_retries) {
    if (yuyv_shm.open_and_map(20 * 1024 * 1024, 5 * 1024 * 1024) == ShmStatus::Success) {
      std::cout << "Consumer: Connected to shared memory!" << std::endl;
      break;
    }
    
    std::cout << "Consumer: Waiting for shared memory... (attempt " << (retry_count + 1) << "/" << max_retries << ")" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    retry_count++;
  }
  
  if (retry_count >= max_retries) {
    std::cerr << "Consumer: Failed to connect to shared memory. Make sure producer is running." << std::endl;
    return 1;
  }

  uint64_t last_frame_version = 0;
  cv::Mat display_frame;
  cv::Mat yuyv_frame;
  
  std::cout << "Starting video consumer..." << std::endl;
  std::cout << "Press 'q' to quit or ESC to exit" << std::endl;

  auto start_time = std::chrono::steady_clock::now();
  uint64_t frames_displayed = 0;

  while (true) {
    // Try to read from shared memory
    uint8_t buffer[5 * 1024 * 1024]; // Buffer to hold the image data (5MB)
    uint32_t width, height, channels;
    size_t data_size;
    uint64_t frame_version;
    ImageFormat format;
    uint8_t frame_type;

    ShmStatus status = yuyv_shm.read_image(buffer, sizeof(buffer), &width, &height, 
                                          &channels, &data_size, &frame_version, &format, &frame_type);

    if (status == ShmStatus::Success && frame_version > last_frame_version) {
      // New frame available
      if (format == ImageFormat::YUYV && width == 1920 && height == 1080) {
        // Create OpenCV Mat from YUYV data
        yuyv_frame = cv::Mat(height, width, CV_8UC2, buffer);
        
        // Convert YUYV to BGR for display
        cv::cvtColor(yuyv_frame, display_frame, cv::COLOR_YUV2BGR_YUYV);
        
        // Resize for display (optional, to fit screen better)
        cv::Mat resized_frame;
        cv::resize(display_frame, resized_frame, cv::Size(960, 540)); // Half size
        
        // Display the frame
        cv::imshow("Consumer - Video Stream", resized_frame);
        
        last_frame_version = frame_version;
        frames_displayed++;
        
        // Print status every 30 frames
        if (frames_displayed % 30 == 0) {
          auto current_time = std::chrono::steady_clock::now();
          auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
          double fps = (elapsed > 0) ? (double)frames_displayed / elapsed : 0.0;
          std::cout << "Consumer: Frame " << frame_version << " displayed (FPS: " << fps << ")" << std::endl;
        }
      } else {
        std::cerr << "Consumer: Unexpected frame format or size" << std::endl;
      }
    } else if (status == ShmStatus::NoDataAvailable) {
      // No new data, wait a bit
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else if (status != ShmStatus::Success) {
      std::cerr << "Consumer: Error reading from shared memory" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Check for exit conditions
    char key = cv::waitKey(1) & 0xFF;
    if (key == 'q' || key == 27) { // 'q' or ESC
      std::cout << "Consumer: Exit requested by user" << std::endl;
      break;
    }
  }

  std::cout << "Cleaning up consumer resources..." << std::endl;
  cv::destroyAllWindows();
  yuyv_shm.unmap_and_close();

  std::cout << "Consumer completed successfully. Total frames displayed: " << frames_displayed << std::endl;
  return 0;
}

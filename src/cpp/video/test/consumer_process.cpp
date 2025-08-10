#include "video/image_shm_manager.h"
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <thread>
#include <vector>



int main() {
  std::cout << "=== Video Consumer (Final Verification: File Save Mode) ==="
            << std::endl;

  const std::string output_dir = "final_output_frames";
  system(("mkdir -p " + output_dir).c_str());
  system(("rm -f " + output_dir + "/*").c_str());
  std::cout << "Consumer: Frames will be saved to '" << output_dir << "'."
            << std::endl;

  ImageShmManager yuyv_shm("yuyv_shm");

  std::cout << "Consumer: Waiting for producer..." << std::endl;

  // 初次连接
  while (yuyv_shm.open_and_map(32 * 1024 * 1024, 10 * 1024 * 1024) !=
         ShmStatus::Success) {
    std::cout << "Consumer: Waiting for shared memory..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::cout << "Consumer: Successfully connected to 'yuyv_shm'!" << std::endl;

  uint64_t last_processed_version = 0;
  int frames_saved_count = 0;
  const int max_frames_to_save = 100;
  std::vector<uint8_t> buffer(10 * 1024 * 1024);

  while (frames_saved_count < max_frames_to_save) {
    uint32_t width, height, channels;
    size_t data_size;
    uint64_t frame_version, timestamp_us;
    ImageFormat format;
    uint8_t frame_type;

    ShmStatus status = yuyv_shm.read_image(
        buffer.data(), buffer.size(), &width, &height, &channels, &data_size,
        &frame_version, &timestamp_us, &format, &frame_type);

    if (status == ShmStatus::Success &&
        frame_version > last_processed_version) {
      last_processed_version = frame_version;
      if (channels != 2) {
        std::cerr << "  WARNING: Unexpected channel count (" << channels
                  << "), expected 2 for YUYV." << std::endl;
        continue;
      }

      try {
        cv::Mat yuyv_frame(height, width, CV_8UC2, buffer.data());
        cv::Mat bgr_frame;
        cv::cvtColor(yuyv_frame, bgr_frame, cv::COLOR_YUV2BGR_YUY2);

        std::string filename =
            output_dir + "/frame_" + std::to_string(frame_version) + ".png";
        if (cv::imwrite(filename, bgr_frame)) {
          std::cout << "Consumer: Saved frame " << frame_version
                    << " (timestamp: " << timestamp_us << ") to " << filename
                    << std::endl;
          frames_saved_count++;
        } else {
          std::cerr << "  FAILURE: cv::imwrite failed." << std::endl;
        }
      } catch (const cv::Exception &e) {
        std::cerr << "  FAILURE: OpenCV exception: " << e.what() << std::endl;
      }
    } else if (status == ShmStatus::NotInitialized ||
               status == ShmStatus::ShmOpenFailed) {
      std::cerr << "Consumer: SHM disconnected, trying to reconnect..."
                << std::endl;
      while (yuyv_shm.open_and_map(32 * 1024 * 1024, 10 * 1024 * 1024) !=
             ShmStatus::Success) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      std::cout << "Consumer: Reconnected to shared memory!" << std::endl;
    } else if (status != ShmStatus::NoDataAvailable) {
      std::cerr << "Consumer: read_image returned status: "
                << shm_status_to_string(status) << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  std::cout << "Consumer: Finished saving " << frames_saved_count
            << " frames. Exiting." << std::endl;
  yuyv_shm.unmap_and_close();
  return 0;
}

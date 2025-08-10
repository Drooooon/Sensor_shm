// ==========================================================
//           src/cpp/video/consumer_gui.cpp
// ==========================================================
#include "video/image_shm_manager.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

int main() {
  std::cout << "=== Video Consumer with GUI (Performance Analysis Mode) ==="
            << std::endl;

  ImageShmManager yuyv_shm("yuyv_shm");

  std::cout << "ConsumerGUI: Waiting for producer to create shared memory..."
            << std::endl;
  while (yuyv_shm.open_and_map(32 * 1024 * 1024, 10 * 1024 * 1024) !=
         ShmStatus::Success) {
    std::cout << "ConsumerGUI: Waiting..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  std::cout << "ConsumerGUI: Successfully connected to shared memory!"
            << std::endl;

  // 创建窗口
  const std::string window_name = "Shared Memory Video Stream";
  try {
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);
  } catch (const cv::Exception &e) {
    std::cerr
        << "FATAL: Failed to create OpenCV window. Environment issue suspected."
        << std::endl;
    std::cerr << "OpenCV Error: " << e.what() << std::endl;
    return 1;
  }

  uint64_t last_processed_version = 0;
  std::vector<uint8_t> buffer(10 * 1024 * 1024);
  cv::Mat yuyv_frame, bgr_frame;

  // 性能统计变量
  auto last_log_time = std::chrono::steady_clock::now();
  int frames_read_since_log = 0;
  int frames_displayed_since_log = 0;
  long long total_processing_time_ms = 0;
  double display_fps_on_window = 0.0;

  std::cout << "ConsumerGUI: Starting video display. Press 'q' or ESC to exit."
            << std::endl;

  while (true) {
    uint32_t width, height, channels;
    size_t data_size;
    uint64_t frame_version, timestamp_us;
    ImageFormat format;
    uint8_t frame_type;

    ShmStatus status = yuyv_shm.read_image(
        buffer.data(), buffer.size(), &width, &height, &channels, &data_size,
        &frame_version, &timestamp_us, &format, &frame_type);

    // ================================================
    // ===              关键逻辑修复              ===
    // ================================================
    if (status == ShmStatus::Success) {
      if (frame_version > last_processed_version) {
        // --- 这是我们期望的正常流程：处理新帧 ---
        frames_read_since_log++;
        last_processed_version = frame_version;

        auto processing_start = std::chrono::steady_clock::now();
        try {
          yuyv_frame = cv::Mat(height, width, CV_8UC2, buffer.data());
          cv::cvtColor(yuyv_frame, bgr_frame, cv::COLOR_YUV2BGR_YUY2);

          std::string fps_text =
              "Display FPS: " + std::to_string((int)display_fps_on_window);
          cv::putText(bgr_frame, fps_text, cv::Point(10, 30),
                      cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);

          cv::imshow(window_name, bgr_frame);

          frames_displayed_since_log++;
        } catch (const cv::Exception &e) {
          std::cerr << "ConsumerGUI: OpenCV processing error on frame "
                    << frame_version << ": " << e.what() << std::endl;
        }
        auto processing_end = std::chrono::steady_clock::now();
        total_processing_time_ms +=
            std::chrono::duration_cast<std::chrono::milliseconds>(
                processing_end - processing_start)
                .count();
      }
      // else {
      //   成功读取，但读到的是一帧旧数据 (frame_version <=
      //   last_processed_version) 我们什么都不做，安静地忽略它，继续下一次循环
      // }
    } else if (status != ShmStatus::NoDataAvailable) {
      // 只有当 status 不是 Success 也不是 NoDataAvailable
      // 时，才是一个真正的错误
      std::cerr
          << "ConsumerGUI: An actual error occurred while reading from SHM: "
          << shm_status_to_string(status) << std::endl;
    }
    // 如果 status 是 NoDataAvailable，我们也不做任何事

    // --- 每 2 秒在终端打印一次详细的性能日志 ---
    auto now = std::chrono::steady_clock::now();
    auto elapsed_ms_since_log =
        std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                              last_log_time)
            .count();

    if (elapsed_ms_since_log >= 2000) {
      double read_fps = frames_read_since_log * 1000.0 / elapsed_ms_since_log;
      display_fps_on_window =
          frames_displayed_since_log * 1000.0 / elapsed_ms_since_log;
      double avg_processing_time =
          (frames_displayed_since_log > 0)
              ? (double)total_processing_time_ms / frames_displayed_since_log
              : 0.0;

      std::cout << "--- Consumer Stats (2s interval) ---" << std::endl;
      std::cout << "  Frames Read from SHM: " << std::fixed
                << std::setprecision(1) << read_fps << " FPS" << std::endl;
      std::cout << "  Frames Displayed:     " << std::fixed
                << std::setprecision(1) << display_fps_on_window << " FPS"
                << std::endl;
      std::cout << "  Avg Processing Time:  " << std::fixed
                << std::setprecision(2) << avg_processing_time << " ms/frame"
                << std::endl;

      last_log_time = now;
      frames_read_since_log = 0;
      frames_displayed_since_log = 0;
      total_processing_time_ms = 0;
    }

    // --- 处理键盘输入 ---
    char key = (char)cv::waitKey(1);
    if (key == 'q' || key == 27) {
      break;
    }

    // 短暂休息，避免在没有新数据或读到旧数据时空转 CPU
    if (status != ShmStatus::Success ||
        frame_version <= last_processed_version) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }

  std::cout << "\nConsumerGUI: Exit signal received. Cleaning up..."
            << std::endl;
  cv::destroyAllWindows();
  yuyv_shm.unmap_and_close();
  std::cout << "ConsumerGUI: Exited cleanly." << std::endl;

  return 0;
}
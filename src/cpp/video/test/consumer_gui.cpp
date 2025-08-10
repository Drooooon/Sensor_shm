// ==========================================================
//      src/cpp/video/test/consumer_gui.cpp (最终动态版)
// ==========================================================
#include "config/config_manager.h"
#include "config/factory.h"
#include "video/image_shm_manager.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>

int main() {
  std::cout << "=== Video Consumer (Dynamic Factory Version) ===" << std::endl;

  try {
    // 1. 加载配置
    ConfigManager::get_instance().load_shm_config(
        "../../../config/shmConfig.json");
    const auto &shm_config = ConfigManager::get_instance().get_shm_config();

    // 2. 连接共享内存
    ImageShmManager shm_transport(shm_config.name);
    std::cout << "ConsumerGUI: Waiting for producer to create shared memory..."
              << std::endl;
    while (shm_transport.open_and_map(shm_config.total_size_bytes,
                                      shm_config.buffer_size_bytes) !=
           ShmStatus::Success) {
      std::cout << "ConsumerGUI: Waiting..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "ConsumerGUI: Successfully connected to shared memory!"
              << std::endl;

    // 3. 创建解码器 Map - 支持动态格式切换
    std::map<ImageFormat, std::unique_ptr<IDecoder>> decoders;
    decoders[ImageFormat::YUYV] = Factory::create_decoder(ImageFormat::YUYV);
    decoders[ImageFormat::MJPG] = Factory::create_decoder(ImageFormat::MJPG);

    // 4. 创建窗口
    const std::string window_name = "Dynamic Video Stream";
    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);

    // 5. 主循环变量
    uint64_t last_processed_version = 0;
    std::vector<uint8_t> buffer(10 * 1024 * 1024);

    // 性能统计
    auto last_log_time = std::chrono::steady_clock::now();
    int frames_processed = 0;
    double display_fps = 0.0;
    ImageFormat last_format = ImageFormat::YUYV; // 跟踪格式变化

    std::cout
        << "ConsumerGUI: Starting video display. Press 'q' or ESC to exit."
        << std::endl;

    while (true) {
      uint32_t width, height, channels;
      size_t data_size;
      uint64_t frame_version, timestamp_us;
      ImageFormat format;
      uint8_t frame_type;

      ShmStatus status = shm_transport.read_image(
          buffer.data(), buffer.size(), &width, &height, &channels, &data_size,
          &frame_version, &timestamp_us, &format, &frame_type);

      if (status == ShmStatus::Success &&
          frame_version > last_processed_version) {
        last_processed_version = frame_version;

        // 检测格式变化
        if (format != last_format) {
          std::cout << "ConsumerGUI: Format changed from " << (int)last_format
                    << " to " << (int)format << std::endl;
          last_format = format;
        }

        // 4. 根据收到的 format 查找正确的解码器
        auto it = decoders.find(format);
        if (it != decoders.end() && it->second) {
          auto processing_start = std::chrono::steady_clock::now();

          try {
            // 5. 使用找到的解码器进行解码
            ImageHeader header = {
                format,    width, height, channels, (uint32_t)data_size,
                frame_type};
            cv::Mat bgr_frame = it->second->decode(buffer.data(), header);

            if (!bgr_frame.empty()) {
              // 添加状态信息到图像
              std::string info_text =
                  "Format: " + std::to_string((int)format) +
                  " | FPS: " + std::to_string((int)display_fps) + " | " +
                  std::to_string(width) + "x" + std::to_string(height);
              cv::putText(bgr_frame, info_text, cv::Point(10, 30),
                          cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0),
                          2);

              cv::imshow(window_name, bgr_frame);
              frames_processed++;
            }
          } catch (const cv::Exception &e) {
            std::cerr << "ConsumerGUI: Decoding error for format "
                      << (int)format << ": " << e.what() << std::endl;
          }
        } else {
          std::cerr << "ConsumerGUI: No decoder found for format "
                    << (int)format << std::endl;
        }
      }

      // 性能统计
      auto now = std::chrono::steady_clock::now();
      auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now - last_log_time)
                            .count();
      if (elapsed_ms >= 2000) {
        display_fps = frames_processed * 1000.0 / elapsed_ms;
        std::cout << "ConsumerGUI: Display FPS: " << std::fixed
                  << std::setprecision(1) << display_fps
                  << ", Format: " << (int)last_format << std::endl;
        last_log_time = now;
        frames_processed = 0;
      }

      // 处理键盘输入
      char key = (char)cv::waitKey(1);
      if (key == 'q' || key == 27)
        break;

      // 避免空转CPU
      if (status != ShmStatus::Success) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }

    // 清理
    cv::destroyAllWindows();
    shm_transport.unmap_and_close();

  } catch (const std::exception &e) {
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "ConsumerGUI: Exited cleanly." << std::endl;
  return 0;
}
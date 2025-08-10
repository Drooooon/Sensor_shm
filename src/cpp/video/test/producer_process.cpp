// ==========================================================
//   src/cpp/video/test/producer_process.cpp (最终重构版)
// ==========================================================
#include "config/config_manager.h"
#include "video/formats/yuyv_processor.h"
#include "video/image_shm_manager.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <opencv2/core/core.hpp> // 仅为 CV_8UC2 常量
#include <thread>

// 全局原子变量，用于优雅地停止程序
std::atomic<bool> g_running(true);

// 信号处理器
void signal_handler(int) {
  std::cout << "\nProducer: Shutdown signal received. Exiting loop..."
            << std::endl;
  g_running.store(false);
}

int main() {
  // 注册信号处理器，以便能响应 Ctrl+C
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "=== Video Producer (Refactored V4L2 Version) ===" << std::endl;

  try {
    // ==================================================
    // 1. 加载配置 (配置层)
    // ==================================================
    // 路径是相对于可执行文件 (build/bin) 的位置
    ConfigManager::get_instance().load_video_config(
        "../../../config/videoConfig.json");
    ConfigManager::get_instance().load_shm_config(
        "../../../config/shmConfig.json");

    // 分别获取不同职责的配置对象
    const auto &v4l2_config = ConfigManager::get_instance().get_v4l2_config();
    const auto &shm_config = ConfigManager::get_instance().get_shm_config();

    // ==================================================
    // 2. 组装模块 (逻辑层)
    // ==================================================
    // a. 根据 V4L2 配置，创建生产者对象
    V4l2Capture producer(v4l2_config);

    // b. 根据共享内存配置，创建传输通道对象
    ImageShmManager shm_transport(shm_config.name);
    shm_transport.create_and_init(shm_config.total_size_bytes,
                                  shm_config.buffer_size_bytes);

    // ==================================================
    // 3. 运行 (执行层)
    // ==================================================
    producer.start_stream();
    std::cout << "Producer stream started. Capturing frames..." << std::endl;

    uint64_t frame_version = 1;
    auto last_log_time = std::chrono::steady_clock::now();
    uint64_t frames_since_log = 0;

    while (g_running.load()) {
      V4l2FrameData frame_data{}; // 初始化为空

      // a. 从生产者捕获一帧 (零拷贝)
      if (producer.capture_frame(frame_data, g_running) && frame_data.data) {

        // b. 将捕获到的帧写入传输通道
        shm_transport.write_image(frame_data.data, frame_data.size,
                                  v4l2_config.width, v4l2_config.height,
                                  2, // channels for YUYV
                                  frame_version++, ImageFormat::YUYV,
                                  CV_8UC2 // type for YUYV
        );
        frames_since_log++;
      }

      // c. 定期打印性能日志
      auto now = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time)
              .count() >= 2) {
        double fps = frames_since_log / 2.0;
        std::cout << "Producer FPS: " << std::fixed << std::setprecision(1)
                  << fps << std::endl;
        frames_since_log = 0;
        last_log_time = now;
      }
    } // end of while loop

    // ==================================================
    // 4. 清理
    // ==================================================
    std::cout << "Capture loop finished. Stopping stream..." << std::endl;
    producer.stop_stream();

  } catch (const std::exception &e) {
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Producer exited cleanly." << std::endl;
  return 0;
}
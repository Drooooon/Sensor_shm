// ==========================================================
//     src/cpp/video/test/producer_process.cpp (最终动态版)
// ==========================================================
#include "config/config_manager.h"
#include "config/factory.h"
#include "video/image_shm_manager.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>

std::atomic<bool> g_running(true);

void signal_handler(int signal) {
  std::cout << "\nProducer: Received signal " << signal
            << ", shutting down gracefully..." << std::endl;
  g_running.store(false);
}

int main() {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "=== Video Producer (Dynamic Factory Version) ===" << std::endl;

  try {
    // 1. 加载配置
    ConfigManager::get_instance().load_video_config(
        "../../../config/videoConfig.json");
    ConfigManager::get_instance().load_shm_config(
        "../../../config/shmConfig.json");
    const auto &v4l2_config = ConfigManager::get_instance().get_v4l2_config();
    const auto &shm_config = ConfigManager::get_instance().get_shm_config();

    std::cout << "Producer: Loaded config - Device: " << v4l2_config.device_path
              << ", PixelFormat: " << v4l2_config.pixel_format_v4l2
              << ", Resolution: " << v4l2_config.width << "x"
              << v4l2_config.height << std::endl;

    // 2. 使用工厂创建捕获器
    auto producer = Factory::create_capture(v4l2_config);
    if (!producer) {
      throw std::runtime_error("Failed to create producer from factory.");
    }

    // 3. 创建传输通道
    ImageShmManager shm_transport(shm_config.name);
    shm_transport.unlink_shm(); // 清理之前的共享内存
    if (shm_transport.create_and_init(
            shm_config.total_size_bytes, shm_config.buffer_size_bytes,
            shm_config.buffer_count) != ShmStatus::Success) {
      throw std::runtime_error("Failed to initialize shared memory.");
    }
    std::cout << "Producer: Shared memory initialized with "
              << shm_config.buffer_count << " buffers." << std::endl;

    // 4. 启动捕获
    producer->start();
    std::cout << "Producer: Started capture stream." << std::endl;

    // 5. 主循环
    uint64_t frame_version = 1;
    uint64_t frames_processed = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (g_running.load()) {
      CapturedFrame frame_data;
      if (producer->capture(frame_data, g_running) && frame_data.data) {
        // 写入共享内存
        ShmStatus status = shm_transport.write_image(
            frame_data.data, frame_data.size, frame_data.width,
            frame_data.height,
            (frame_data.format == ImageFormat::YUYV)
                ? 2
                : 3, // channels approximation
            frame_version++, frame_data.format, frame_data.cv_type);

        if (status == ShmStatus::Success) {
          frames_processed++;
          if (frames_processed % 30 == 0) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            double fps =
                frames_processed * 1000.0 /
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed)
                    .count();
            std::cout << "Producer: Processed " << frames_processed
                      << " frames, FPS: " << std::fixed << std::setprecision(1)
                      << fps << ", Format: " << (int)frame_data.format
                      << ", Size: " << frame_data.size << " bytes" << std::endl;
          }
        } else {
          std::cerr << "Producer: Failed to write frame to SHM" << std::endl;
        }
      }

      if (!g_running.load())
        break;
    }

    // 6. 清理
    producer->stop();
    shm_transport.unmap_and_close();
    shm_transport.unlink_shm();

  } catch (const std::exception &e) {
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Producer exited cleanly." << std::endl;
  return 0;
}
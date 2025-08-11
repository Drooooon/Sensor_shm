/**
 * @file producer_process.cpp
 * @brief 视频生产者进程主程序
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该程序作为视频数据的生产者，从V4L2摄像头设备捕获视频帧，
 * 并通过共享内存将数据传输给消费者进程。使用我们重构的V4l2Capture
 * 类提供高效、可靠的视频捕获服务。
 *
 * 主要功能：
 * - 从配置文件加载共享内存和视频参数
 * - 使用V4l2Capture类直接访问摄像头设备
 * - 初始化共享内存缓冲区
 * - 连续捕获视频帧并写入共享内存
 * - 实时性能监控和日志输出
 * - 优雅的信号处理和资源清理
 */

#include "config/config_manager.h"
#include "config/factory.h"
#include "formats/capture_interface.h"
#include "image_shm_manager.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

/// 全局运行状态标志，用于优雅退出
std::atomic<bool> g_running(true);

/**
 * @brief 信号处理函数
 * @param signal 接收到的信号编号
 *
 * 处理SIGINT和SIGTERM信号，设置运行标志为false，
 * 实现程序的优雅退出机制。
 */
void signal_handler(int signal) {
  std::cout << "\nProducer: Received signal " << signal << ", shutting down..."
            << std::endl;
  g_running.store(false);
}

/**
 * @brief 将V4L2像素格式转换为ImageFormat枚举
 * @param pixel_format V4L2像素格式常量
 * @return ImageFormat 对应的图像格式枚举
 */
ImageFormat v4l2_format_to_image_format(uint32_t pixel_format) {
  if (pixel_format == 0x56595559) { // V4L2_PIX_FMT_YUYV 'YUYV' = 1448695129
    return ImageFormat::YUYV;
  } else if (pixel_format ==
             0x47504a4d) { // V4L2_PIX_FMT_MJPEG 'MJPG' = 1196444237
    return ImageFormat::MJPG;
  } else {
    std::cout << "Producer: Warning - Unknown pixel format: " << pixel_format
              << ", defaulting to MJPG" << std::endl;
    return ImageFormat::MJPG; // 默认使用MJPG
  }
}

/**
 * @brief 主函数 - 视频生产者进程入口
 * @return int 程序退出码，0表示正常退出，非0表示异常退出
 *
 * 程序执行流程：
 * 1. 注册信号处理函数
 * 2. 加载配置文件
 * 3. 使用Factory创建V4L2捕获器
 * 4. 初始化共享内存管理器
 * 5. 启动V4L2捕获流
 * 6. 进入主循环：捕获帧 -> 写入共享内存 -> 更新统计
 * 7. 优雅清理资源并退出
 *
 * 技术特点：
 * - 完全基于V4L2 API，避免GStreamer依赖
 * - 从配置文件读取所有参数
 * - 支持多种视频格式和分辨率
 * - 实时FPS监控，每2秒输出一次统计
 * - 异常情况自动重试和恢复
 */
int main() {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "=== Video Producer (V4L2 Direct Version) ===" << std::endl;

  try {
    // 1. 加载配置文件
    ConfigManager::get_instance().load_video_config(
        "../../../config/videoConfig.json");
    ConfigManager::get_instance().load_shm_config(
        "../../../config/shmConfig.json");

    const auto &video_config = ConfigManager::get_instance().get_v4l2_config();
    const auto &shm_config = ConfigManager::get_instance().get_shm_config();

    std::cout << "Producer: Loaded config - Device: "
              << video_config.device_path
              << ", Format: " << video_config.pixel_format_v4l2 << " (0x"
              << std::hex << video_config.pixel_format_v4l2 << std::dec << ")"
              << ", Resolution: " << video_config.width << "x"
              << video_config.height << ", SHM: " << shm_config.name
              << std::endl;

    // 2. 使用工厂创建V4L2捕获器
    auto capture = Factory::create_capture(video_config);
    if (!capture) {
      std::cerr << "FATAL: Failed to create V4L2 capture device." << std::endl;
      return 1;
    }
    std::cout << "Producer: V4L2 capture device created successfully."
              << std::endl;

    // 3. 初始化共享内存
    ImageShmManager shm_manager(shm_config.name);
    shm_manager.unlink_shm(); // 清理之前的共享内存

    size_t total_size = shm_config.total_size_bytes;
    size_t buffer_size = shm_config.buffer_size_bytes;
    int buffer_count = shm_config.buffer_count;

    if (shm_manager.create_and_init(total_size, buffer_size, buffer_count) !=
        ShmStatus::Success) {
      std::cerr << "FATAL: Failed to initialize shared memory '"
                << shm_config.name << "'." << std::endl;
      return 1;
    }
    std::cout << "Producer: Shared memory '" << shm_config.name
              << "' initialized with " << buffer_count << " buffers."
              << std::endl;

    // 4. 启动V4L2捕获流
    capture->start();
    std::cout << "Producer: V4L2 capture stream started successfully."
              << std::endl;

    // 5. 主循环
    uint64_t frame_version = 1;
    auto last_log_time = std::chrono::steady_clock::now();
    uint64_t frames_since_log = 0;
    uint64_t total_frames = 0;
    ImageFormat image_format =
        v4l2_format_to_image_format(video_config.pixel_format_v4l2);

    std::cout << "Producer: Starting capture loop with format "
              << (int)image_format << " ("
              << (image_format == ImageFormat::YUYV ? "YUYV" : "MJPG") << ")"
              << std::endl;

    CapturedFrame frame;
    while (g_running.load()) {
      // 从V4L2设备捕获一帧
      bool capture_success = capture->capture(frame, g_running);

      if (!capture_success) {
        if (!g_running.load())
          break; // 正常退出
        std::cerr << "Producer: Warning - Failed to capture frame, retrying..."
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        continue;
      }

      // 检查帧数据有效性
      if (!frame.data || frame.size == 0) {
        std::cerr << "Producer: Warning - Invalid frame data, skipping..."
                  << std::endl;
        continue;
      }

      // 写入共享内存
      ShmStatus status = shm_manager.write_image(
          frame.data, frame.size, frame.width, frame.height,
          (frame.format == ImageFormat::YUYV) ? 2 : 1, // YUYV=2通道，MJPG=1通道
          frame_version, frame.format, frame.cv_type);

      if (status == ShmStatus::Success) {
        frame_version++;
        frames_since_log++;
        total_frames++;

        // 性能统计输出
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                             last_log_time)
                .count() >= 2) {
          double fps = frames_since_log / 2.0;
          std::cout << "Producer: FPS: " << std::fixed << std::setprecision(1)
                    << fps << " | Total Frames: " << total_frames
                    << " | Format: " << (int)frame.format
                    << " | Size: " << frame.size << " bytes" << std::endl;
          frames_since_log = 0;
          last_log_time = now;
        }

      } else {
        std::cerr << "Producer: Failed to write frame " << frame_version
                  << " to shared memory (Status: " << (int)status << ")"
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }

    // 6. 清理资源
    std::cout << "\nProducer: Shutting down..." << std::endl;
    capture->stop();
    std::cout << "Producer: V4L2 capture stopped." << std::endl;

    shm_manager.unmap_and_close();
    std::cout << "Producer: Shared memory unmapped." << std::endl;

    // 注意：不调用 unlink_shm()，让消费者进程继续访问数据
    std::cout << "Producer: Exited cleanly after processing " << total_frames
              << " frames." << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
    g_running.store(false);
    return 1;
  }

  return 0;
}
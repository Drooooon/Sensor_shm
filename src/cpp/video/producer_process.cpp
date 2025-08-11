/**
 * @file producer_process.cpp
 * @brief 视频生产者进程主程序
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该程序作为视频数据的生产者，从摄像头设备捕获视频帧，
 * 并通过共享内存将数据传输给消费者进程。使用GStreamer作为
 * 视频捕获后端，支持多种视频格式和设备。
 *
 * 主要功能：
 * - 初始化共享内存缓冲区
 * - 配置GStreamer视频捕获管道
 * - 连续捕获视频帧并写入共享内存
 * - 实时性能监控和日志输出
 * - 优雅的信号处理和资源清理
 */

#include "image_shm_manager.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <thread>

/// 全局运行状态标志，用于优雅退出
std::atomic<bool> g_running(true);

/**
 * @brief 将OpenCV Mat类型转换为可读字符串
 * @param type OpenCV Mat类型常量
 * @return std::string 类型描述字符串
 *
 * 用于调试和日志输出，将数字类型常量转换为易读的字符串形式。
 */
std::string mat_type_to_string(int type) {
  switch (type) {
  case CV_8UC1:
    return "CV_8UC1";
  case CV_8UC2:
    return "CV_8UC2";
  case CV_8UC3:
    return "CV_8UC3";
  case CV_8UC4:
    return "CV_8UC4";
  default:
    return "Unknown(" + std::to_string(type) + ")";
  }
}

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
 * @brief 主函数 - 视频生产者进程入口
 * @return int 程序退出码，0表示正常退出，非0表示异常退出
 *
 * 程序执行流程：
 * 1. 注册信号处理函数
 * 2. 初始化共享内存管理器
 * 3. 配置并打开GStreamer视频捕获管道
 * 4. 进入主循环：捕获帧 -> 写入共享内存 -> 更新统计
 * 5. 优雅清理资源并退出
 *
 * 技术特点：
 * - 使用32MB共享内存，3个10MB缓冲区
 * - GStreamer管道支持YUY2格式1280x720分辨率
 * - 实时FPS监控，每2秒输出一次统计
 * - 异常情况自动重试和恢复
 */
int main() {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "=== Video Producer (Optimized Version) ===" << std::endl;

  ImageShmManager yuyv_shm("yuyv_shm");
  yuyv_shm.unlink_shm();
  if (yuyv_shm.create_and_init(32 * 1024 * 1024, 10 * 1024 * 1024, 3) !=
      ShmStatus::Success) {
    std::cerr << "FATAL: Failed to initialize shared memory." << std::endl;
    return 1;
  }
  std::cout << "Producer: Shared memory initialized with 3 buffers."
            << std::endl;

  // 使用你已经验证成功的 /dev/video0
  std::string pipeline = "v4l2src device=/dev/video0 ! "
                         "video/x-raw,format=YUY2,width=1280,height=720 ! "
                         "appsink drop=true sync=false";
  cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);

  if (!cap.isOpened()) {
    std::cerr << "FATAL: Failed to open GStreamer pipeline." << std::endl;
    return 1;
  }
  std::cout << "Producer: Camera pipeline opened successfully." << std::endl;

  uint64_t frame_version = 1;
  cv::Mat frame;
  auto last_log_time = std::chrono::steady_clock::now();
  uint64_t frames_since_log = 0;

  while (g_running.load()) {
    if (!cap.read(frame) || frame.empty()) {
      std::cerr << "Producer: Warning - Failed to read frame." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    ShmStatus status = yuyv_shm.write_image(
        frame.data, frame.total() * frame.elemSize(), frame.cols, frame.rows,
        frame.channels(), frame_version, ImageFormat::YUYV, frame.type());

    if (status == ShmStatus::Success) {
      frame_version++;
      frames_since_log++;

      auto now = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time)
              .count() >= 2) {
        double fps = frames_since_log / 2.0;
        std::cout << "Producer: FPS: " << std::fixed << std::setprecision(1)
                  << fps << " | Total Frames: " << frame_version - 1
                  << std::endl;
        frames_since_log = 0;
        last_log_time = now;
      }

    } else {
      std::cerr << "Producer: Failed to write frame " << frame_version
                << " to SHM" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  std::cout << "\nProducer: Shutting down..." << std::endl;
  cap.release();
  yuyv_shm.unmap_and_close();
  yuyv_shm.unlink_shm();
  std::cout << "Producer: Exited cleanly." << std::endl;
  return 0;
}
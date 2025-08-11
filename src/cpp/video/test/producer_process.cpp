/**
 * @file producer_process.cpp
 * @brief 视频生产者进程主程序 (测试版本)
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 2.0
 * @copyright Copyright (c) 2025 SensorComm Team
 *
 * @details
 * 该程序作为视频数据的生产者进程，负责从V4L2摄像头设备捕获视频帧，
 * 并通过高性能共享内存将数据传输给消费者进程。采用工厂模式设计，
 * 支持动态格式切换和多种视频设备类型。
 *
 * ## 主要功能
 * - 🔧 从JSON配置文件加载视频和共享内存参数
 * - 📹 使用Factory模式创建V4L2捕获器实例
 * - 🚀 初始化高性能共享内存缓冲区系统
 * - 📊 连续捕获视频帧并写入共享内存
 * - 📈 实时性能监控和FPS统计输出
 * - 🛡️ 优雅的信号处理和资源清理机制
 *
 * ## 技术特点
 * - **零拷贝传输**: 基于共享内存的高效数据传输
 * - **多格式支持**: YUYV、MJPEG等主流视频格式
 * - **工厂模式**: 灵活的设备创建和管理机制
 * - **异常安全**: RAII资源管理和完善的错误处理
 * - **实时监控**: 每30帧输出一次性能统计信息
 *
 * ## 使用示例
 * @code{.bash}
 * # 编译项目
 * make video
 *
 * # 运行生产者进程
 * ./build/bin/producer_process
 *
 * # 或使用测试脚本
 * ./test/test_streaming.sh
 * @endcode
 *
 * @see consumer_gui.cpp 消费者GUI程序
 * @see consumer_process.cpp 消费者进程程序
 * @see ImageShmManager 共享内存管理器
 * @see Factory 工厂模式实现
 *
 * @warning 运行前请确保摄像头设备可用且配置文件路径正确
 * @note 本程序需要读写 /dev/shm 权限和摄像头设备访问权限
 */

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

/**
 * @brief 全局运行状态标志
 * @details 用于优雅退出的原子布尔变量，在信号处理函数中设置为false，
 *          主循环检查此标志以决定是否继续运行。
 */
std::atomic<bool> g_running(true);

/**
 * @brief 信号处理函数
 * @param signal 接收到的信号编号 (通常是SIGINT或SIGTERM)
 *
 * @details
 * 处理系统信号，实现程序的优雅退出机制。当收到中断信号时，
 * 设置全局运行标志为false，让主循环安全退出并清理资源。
 *
 * 支持的信号：
 * - SIGINT (Ctrl+C)
 * - SIGTERM (终止信号)
 *
 * @note 该函数是异步安全的，只使用原子操作
 * @warning 在信号处理函数中避免使用非异步安全的函数
 */
void signal_handler(int signal) {
  std::cout << "\nProducer: Received signal " << signal
            << ", shutting down gracefully..." << std::endl;
  g_running.store(false);
}

/**
 * @brief 主函数 - 视频生产者进程入口点
 * @return int 程序退出码
 * @retval 0 程序正常退出
 * @retval 1 程序异常退出 (配置加载失败、设备初始化失败等)
 *
 * @details
 * 程序执行流程：
 * 1. **信号注册**: 注册SIGINT和SIGTERM信号处理函数
 * 2. **配置加载**: 从JSON文件加载视频和共享内存配置
 * 3. **设备创建**: 使用Factory模式创建V4L2捕获器
 * 4. **内存初始化**: 创建并初始化共享内存缓冲区
 * 5. **启动捕获**: 开始V4L2视频流捕获
 * 6. **主循环**: 连续捕获帧并写入共享内存
 * 7. **资源清理**: 停止捕获、释放内存、清理资源
 *
 * ## 性能特点
 * - 📊 **实时统计**: 每处理30帧输出一次FPS统计
 * - ⚡ **高效传输**: 零拷贝共享内存数据传输
 * - 🔄 **动态适应**: 自动适应不同视频格式和分辨率
 * - 🛡️ **异常处理**: 完善的错误处理和资源清理机制
 *
 * ## 配置依赖
 * 程序依赖以下配置文件：
 * - `../../../config/videoConfig.json`: 视频设备配置
 * - `../../../config/shmConfig.json`: 共享内存配置
 *
 * @throws std::runtime_error 当配置加载失败或设备初始化失败时抛出
 * @throws std::exception 其他系统级异常
 *
 * @see ConfigManager::load_video_config() 视频配置加载
 * @see ConfigManager::load_shm_config() 共享内存配置加载
 * @see Factory::create_capture() 捕获器工厂方法
 * @see ImageShmManager::create_and_init() 共享内存初始化
 *
 * @par 示例输出
 * @code
 * === Video Producer (Dynamic Factory Version) ===
 * Producer: Loaded config - Device: /dev/video0, PixelFormat: 1196444237,
 * Resolution: 1280x720 Producer: Shared memory initialized with 3 buffers.
 * Producer: Started capture stream.
 * Producer: Processed 30 frames, FPS: 29.8, Format: 3, Size: 184320 bytes
 * Producer: Processed 60 frames, FPS: 30.1, Format: 3, Size: 184320 bytes
 * Producer exited cleanly.
 * @endcode
 */
int main() {
  // 注册信号处理函数，确保程序能够优雅退出
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  std::cout << "=== Video Producer (Dynamic Factory Version) ===" << std::endl;

  try {
    // ================================================================
    // 1. 配置加载阶段
    // ================================================================

    // 加载视频设备配置 (设备路径、分辨率、格式等)
    ConfigManager::get_instance().load_video_config(
        "../../../config/videoConfig.json");

    // 加载共享内存配置 (内存大小、缓冲区数量等)
    ConfigManager::get_instance().load_shm_config(
        "../../../config/shmConfig.json");

    const auto &v4l2_config = ConfigManager::get_instance().get_v4l2_config();
    const auto &shm_config = ConfigManager::get_instance().get_shm_config();

    // 输出加载的配置信息，便于调试
    std::cout << "Producer: Loaded config - Device: " << v4l2_config.device_path
              << ", PixelFormat: " << v4l2_config.pixel_format_v4l2
              << ", Resolution: " << v4l2_config.width << "x"
              << v4l2_config.height << std::endl;

    // ================================================================
    // 2. 设备创建阶段
    // ================================================================

    /**
     * 使用工厂模式创建V4L2捕获器实例
     * Factory::create_capture() 会根据配置自动选择合适的捕获器实现
     */
    auto producer = Factory::create_capture(v4l2_config);
    if (!producer) {
      throw std::runtime_error("Failed to create producer from factory.");
    }

    // ================================================================
    // 3. 共享内存初始化阶段
    // ================================================================

    /**
     * 创建共享内存传输通道
     * ImageShmManager 提供高效的图像数据共享内存管理
     */
    ImageShmManager shm_transport(shm_config.name);
    shm_transport.unlink_shm(); // 清理之前可能残留的共享内存

    // 初始化共享内存缓冲区系统
    if (shm_transport.create_and_init(
            shm_config.total_size_bytes, shm_config.buffer_size_bytes,
            shm_config.buffer_count) != ShmStatus::Success) {
      throw std::runtime_error("Failed to initialize shared memory.");
    }

    std::cout << "Producer: Shared memory initialized with "
              << shm_config.buffer_count << " buffers." << std::endl;

    // ================================================================
    // 4. 启动捕获阶段
    // ================================================================

    /**
     * 启动V4L2视频流捕获
     * 这将初始化摄像头设备并开始数据流
     */
    producer->start();
    std::cout << "Producer: Started capture stream." << std::endl;

    // ================================================================
    // 5. 主循环 - 视频捕获与传输
    // ================================================================

    uint64_t frame_version = 1; ///< 帧版本号，用于消费者检测新帧
    uint64_t frames_processed = 0; ///< 已处理帧数统计
    auto start_time = std::chrono::steady_clock::now(); ///< 性能统计起始时间

    /**
     * @brief 主处理循环
     *
     * 循环执行以下操作：
     * 1. 从V4L2设备捕获一帧数据
     * 2. 检查帧数据的有效性
     * 3. 将帧数据写入共享内存
     * 4. 更新统计信息并定期输出
     * 5. 检查退出信号
     */
    while (g_running.load()) {
      CapturedFrame frame_data;

      // 从捕获器获取一帧数据
      if (producer->capture(frame_data, g_running) && frame_data.data) {

        /**
         * @brief 写入共享内存
         *
         * 根据视频格式计算通道数：
         * - YUYV格式: 2通道 (Y + UV)
         * - 其他格式: 3通道 (RGB或类似)
         */
        ShmStatus status = shm_transport.write_image(
            frame_data.data, frame_data.size, frame_data.width,
            frame_data.height,
            (frame_data.format == ImageFormat::YUYV) ? 2 : 3, // 通道数近似值
            frame_version++, frame_data.format, frame_data.cv_type);

        if (status == ShmStatus::Success) {
          frames_processed++;

          /**
           * @brief 性能统计输出
           *
           * 每处理30帧输出一次统计信息，包括：
           * - 总处理帧数
           * - 实时FPS
           * - 当前帧格式
           * - 帧数据大小
           */
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

      // 检查退出信号，确保响应用户中断
      if (!g_running.load())
        break;
    }

    // ================================================================
    // 6. 资源清理阶段
    // ================================================================

    /**
     * @brief 优雅关闭和资源清理
     *
     * 按照正确的顺序清理资源：
     * 1. 停止视频捕获流
     * 2. 解除共享内存映射
     * 3. 清理共享内存对象
     */
    producer->stop();                // 停止V4L2捕获流
    shm_transport.unmap_and_close(); // 解除内存映射
    shm_transport.unlink_shm();      // 清理共享内存对象

  } catch (const std::exception &e) {
    /**
     * @brief 异常处理
     *
     * 捕获并输出所有异常，确保程序不会崩溃
     * 返回错误码1表示异常退出
     */
    std::cerr << "FATAL ERROR: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Producer exited cleanly." << std::endl;
  return 0;
}
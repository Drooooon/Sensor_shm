/**
 * @file v4l2_capture.h
 * @brief V4L2视频设备捕获器实现
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件实现了基于Linux V4L2（Video for Linux 2）API的视频捕获器，
 * 支持USB摄像头和其他V4L2兼容设备的视频数据获取。
 */

#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include "capture_interface.h"
#include "config/config_manager.h"
#include <atomic>
#include <cstdint>
#include <string>

/**
 * @brief V4L2视频捕获器类
 *
 * 实现 ICapture 接口的 V4L2 捕获器，专门用于Linux系统下的视频设备访问。
 * 使用内存映射（mmap）方式进行高效的数据传输，支持多种像素格式。
 *
 * 主要特性：
 * - 支持YUYV、MJPEG等常见格式
 * - 使用内存映射避免数据拷贝
 * - 多缓冲区机制确保流畅采集
 * - 线程安全的启动/停止控制
 * - 自动设备资源管理
 */
class V4l2Capture : public ICapture {
public:
  /**
   * @brief 构造函数
   * @param config V4L2配置参数，包含设备路径、分辨率等信息
   *
   * 根据配置初始化V4L2捕获器，但不立即打开设备。
   * 设备的实际初始化在调用start()时进行。
   */
  explicit V4l2Capture(const V4l2Config &config);

  /**
   * @brief 析构函数
   *
   * 自动停止捕获并清理所有资源，包括：
   * - 关闭设备文件描述符
   * - 释放内存映射缓冲区
   * - 清理内部状态
   */
  ~V4l2Capture();

  /**
   * @brief 启动视频捕获
   * @throws std::runtime_error 当设备打开或初始化失败时抛出异常
   *
   * 执行完整的设备初始化流程：
   * 1. 打开V4L2设备文件
   * 2. 设置视频格式和分辨率
   * 3. 分配和映射缓冲区
   * 4. 开始视频流
   */
  void start() override;

  /**
   * @brief 停止视频捕获
   *
   * 安全地停止视频流并释放资源：
   * 1. 停止视频流
   * 2. 解除内存映射
   * 3. 关闭设备文件
   * 4. 重置内部状态
   */
  void stop() override;

  /**
   * @brief 捕获一帧视频数据
   * @param out_frame 输出参数，接收捕获的帧数据
   * @param running 原子布尔标志，用于外部控制捕获循环
   * @return bool true表示成功捕获帧，false表示应停止捕获
   *
   * 从V4L2设备队列中取出一帧数据。该方法会阻塞等待新帧到达，
   * 但会定期检查running标志以支持优雅退出。
   *
   * @note 返回的帧数据指针指向内部缓冲区，在下次调用前有效
   * @warning 必须在调用start()之后使用
   */
  bool capture(CapturedFrame &out_frame, std::atomic<bool> &running) override;

  // 禁用拷贝构造和赋值，确保资源管理安全
  V4l2Capture(const V4l2Capture &) = delete;
  V4l2Capture &operator=(const V4l2Capture &) = delete;

private:
  /**
   * @brief 内部缓冲区结构体声明
   *
   * 存储V4L2内存映射缓冲区的信息，包括地址和大小。
   */
  struct Buffer;

  /**
   * @brief V4L2系统调用包装器
   * @param request ioctl请求码
   * @param arg 请求参数指针
   * @throws std::runtime_error 当系统调用失败时抛出异常
   *
   * 封装ioctl调用并处理EINTR中断，确保系统调用的可靠性。
   */
  void xioctl(unsigned long request, void *arg);

  /**
   * @brief 打开V4L2设备文件
   * @throws std::runtime_error 当设备无法打开时抛出异常
   */
  void open_device();

  /**
   * @brief 初始化视频格式设置
   * @throws std::runtime_error 当格式设置失败时抛出异常
   *
   * 根据配置设置设备的像素格式、分辨率等参数。
   */
  void init_format();

  /**
   * @brief 初始化内存映射缓冲区
   * @throws std::runtime_error 当缓冲区分配失败时抛出异常
   *
   * 请求设备分配指定数量的缓冲区并映射到用户空间。
   */
  void init_mmap();

private:
  V4l2Config config_;     ///< V4L2配置参数
  int fd_;                ///< 设备文件描述符
  Buffer *buffers_;       ///< 内存映射缓冲区数组
  uint32_t buffer_count_; ///< 缓冲区数量
  bool is_streaming_;     ///< 是否正在流式传输标志

  CapturedFrame current_frame_; ///< 当前帧数据缓存，用于实现capture()方法
};

#endif // V4L2_CAPTURE_H
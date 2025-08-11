/**
 * @file capture_interface.h
 * @brief 视频捕获接口定义
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件定义了通用的视频捕获接口，提供与具体硬件无关的抽象层。
 * 支持多种视频设备和格式的统一访问。
 */

#ifndef CAPTURE_INTERFACE_H
#define CAPTURE_INTERFACE_H

#include "config/config_manager.h"
#include "video/image_shm_manager.h"
#include <atomic>
#include <cstdint>

/**
 * @brief 捕获帧数据结构
 *
 * 封装了从视频设备捕获的单帧数据及其元数据信息。
 * 包含原始数据指针、尺寸、格式等关键信息。
 */
struct CapturedFrame {
  const uint8_t *data; ///< 帧数据指针，指向原始像素数据
  size_t size;         ///< 帧数据大小（字节）
  uint32_t width;      ///< 图像宽度（像素）
  uint32_t height;     ///< 图像高度（像素）
  ImageFormat format;  ///< 捕获到的原始图像格式
  uint8_t cv_type;     ///< 对应的 OpenCV 数据类型常量
};

/**
 * @brief 视频捕获接口类
 *
 * 定义了视频设备捕获的通用接口，所有具体的捕获实现类都应继承此接口。
 * 提供了设备启动、停止和帧捕获的标准方法。
 *
 * 使用多态设计模式，支持不同类型的视频设备：
 * - V4L2设备（Linux摄像头）
 * - 网络摄像头
 * - 文件视频流等
 */
class ICapture {
public:
  /**
   * @brief 虚析构函数，确保派生类正确析构
   */
  virtual ~ICapture() = default;

  /**
   * @brief 启动视频捕获
   * @throws std::runtime_error 当设备无法启动时抛出异常
   *
   * 初始化视频设备，分配必要的缓冲区，开始捕获过程。
   * 调用此方法后，设备进入就绪状态，可以开始捕获帧数据。
   */
  virtual void start() = 0;

  /**
   * @brief 停止视频捕获
   *
   * 停止捕获过程，释放设备资源，清理缓冲区。
   * 调用此方法后，设备返回非活动状态。
   */
  virtual void stop() = 0;

  /**
   * @brief 捕获一帧数据
   * @param out_frame 输出参数，接收捕获的帧数据
   * @param running 原子布尔标志，用于外部控制捕获循环
   * @return bool true表示成功捕获，false表示失败或应停止
   *
   * 从视频设备获取一帧数据。该方法可能阻塞等待新帧到达。
   * running参数允许外部线程安全地中断捕获过程。
   *
   * @note 该方法通常在循环中调用，实现连续的视频捕获
   * @warning 调用前必须确保已调用start()方法
   */
  virtual bool capture(CapturedFrame &out_frame,
                       std::atomic<bool> &running) = 0;
};

#endif // CAPTURE_INTERFACE_H
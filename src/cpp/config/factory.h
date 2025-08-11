/**
 * @file factory.h
 * @brief 工厂模式实现头文件，提供对象创建接口
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件定义了工厂类，使用工厂模式创建视频捕获和解码器对象。
 * 根据配置参数自动选择和创建合适的实现类。
 */

#ifndef FACTORY_H
#define FACTORY_H

#include "video/formats/capture_interface.h"
#include "video/formats/decoder_interface.h"
#include "video/image_shm_manager.h"
#include <memory>

/**
 * @brief 工厂类，负责创建视频处理相关对象
 *
 * 使用工厂模式统一管理对象创建逻辑，根据配置参数和格式类型
 * 自动选择和实例化合适的捕获器和解码器实现。
 */
class Factory {
public:
  /**
   * @brief 创建视频捕获器对象
   * @param config V4L2配置参数，包含设备路径、分辨率等信息
   * @return std::unique_ptr<ICapture> 指向捕获器接口的智能指针
   * @throws std::runtime_error 当无法创建捕获器时抛出异常
   *
   * 根据V4L2配置中的像素格式和设备参数，创建相应的视频捕获器实例。
   * 目前支持V4L2标准捕获设备。
   */
  static std::unique_ptr<ICapture> create_capture(const V4l2Config &config);

  /**
   * @brief 创建图像解码器对象
   * @param format 图像格式枚举值
   * @return std::unique_ptr<IDecoder> 指向解码器接口的智能指针
   * @throws std::runtime_error 当格式不支持时抛出异常
   *
   * 根据指定的图像格式创建相应的解码器实例。
   * 支持的格式包括YUYV、MJPEG等常见视频格式。
   */
  static std::unique_ptr<IDecoder> create_decoder(ImageFormat format);
};

#endif // FACTORY_H
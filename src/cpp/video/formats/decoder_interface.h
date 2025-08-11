/**
 * @file decoder_interface.h
 * @brief 图像解码器接口定义
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件定义了通用的图像解码器接口，用于将不同格式的原始图像数据
 * 转换为OpenCV可处理的标准BGR格式。
 */

#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "video/image_shm_manager.h"
#include <opencv2/opencv.hpp>

/**
 * @brief 图像解码器接口类
 *
 * 定义了图像解码的通用接口，所有具体的解码实现类都应继承此接口。
 * 负责将各种原始图像格式（如YUYV、MJPEG等）转换为OpenCV标准的BGR格式。
 *
 * 支持的解码格式包括但不限于：
 * - YUYV (YUV422格式)
 * - MJPEG (Motion JPEG压缩格式)
 * - 其他常见的视频编码格式
 */
class IDecoder {
public:
  /**
   * @brief 虚析构函数，确保派生类正确析构
   */
  virtual ~IDecoder() = default;

  /**
   * @brief 解码图像数据
   * @param data 原始图像数据指针
   * @param header 图像头部信息，包含尺寸、格式等元数据
   * @return cv::Mat 解码后的BGR格式OpenCV矩阵
   * @throws std::runtime_error 当解码失败时抛出异常
   *
   * 将从共享内存读取的原始图像数据解码为OpenCV可处理的BGR格式。
   * 该方法会根据header中的格式信息选择合适的解码算法。
   *
   * @note 返回的Mat对象包含解码后的图像数据，调用者负责管理其生命周期
   * @warning data指针必须有效且指向完整的图像数据
   */
  virtual cv::Mat decode(const uint8_t *data, const ImageHeader &header) = 0;
};

#endif // DECODER_INTERFACE_H
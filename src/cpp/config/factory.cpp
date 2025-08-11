/**
 * @file factory.cpp
 * @brief 工厂模式实现文件
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 实现工厂类的对象创建逻辑，根据配置和格式参数
 * 自动选择和实例化合适的捕获器和解码器。
 */

#include "factory.h"
#include "video/formats/mjpg_decoder.h"
#include "video/formats/v4l2_capture.h"
#include "video/formats/yuyv_decoder.h"
#include <stdexcept>

std::unique_ptr<ICapture> Factory::create_capture(const V4l2Config &config) {
  // 目前只支持 V4L2 捕获，未来可扩展其他类型
  return std::make_unique<V4l2Capture>(config);
}

std::unique_ptr<IDecoder> Factory::create_decoder(ImageFormat format) {
  switch (format) {
  case ImageFormat::YUYV:
    return std::make_unique<YuyvDecoder>();
  case ImageFormat::MJPG:
    return std::make_unique<MjpgDecoder>();
  case ImageFormat::BGR:
    // BGR 格式不需要解码
    throw std::runtime_error("Factory Error: BGR format doesn't need decoder");
  case ImageFormat::H264:
    // H264 解码器暂未实现
    throw std::runtime_error("Factory Error: H264 decoder not implemented");
  default:
    throw std::runtime_error("Factory Error: Unsupported format for decoder");
  }
}
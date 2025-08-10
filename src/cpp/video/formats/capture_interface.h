#ifndef CAPTURE_INTERFACE_H
#define CAPTURE_INTERFACE_H

#include "config/config_manager.h"
#include "video/image_shm_manager.h"
#include <atomic>
#include <cstdint>

// 通用的帧数据结构
struct CapturedFrame {
  const uint8_t *data;
  size_t size;
  uint32_t width;
  uint32_t height;
  ImageFormat format; // 捕获到的原始格式
  uint8_t cv_type;    // 对应的 OpenCV 类型
};

class ICapture {
public:
  virtual ~ICapture() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool capture(CapturedFrame &out_frame,
                       std::atomic<bool> &running) = 0;
};

#endif // CAPTURE_INTERFACE_H
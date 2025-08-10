// ==========================================================
//    src/cpp/video/formats/yuyv_processor.h (最终修复版)
// ==========================================================
#ifndef YUYV_PROCESSOR_H
#define YUYV_PROCESSOR_H

#include "config/config_manager.h"
#include <atomic>
#include <cstdint>
#include <string>

struct V4l2FrameData {
  const uint8_t *data;
  size_t size;
  uint32_t width;  // <--- 新增
  uint32_t height; // <--- 新增
};

// *** 统一使用 V4l2Capture 这个名字 ***
class V4l2Capture {
public:
  explicit V4l2Capture(const V4l2Config &config);
  ~V4l2Capture();

  void start_stream();
  void stop_stream();
  bool capture_frame(V4l2FrameData &out_frame, std::atomic<bool> &running);

  V4l2Capture(const V4l2Capture &) = delete;
  V4l2Capture &operator=(const V4l2Capture &) = delete;

private:
  struct Buffer;
  void xioctl(unsigned long request, void *arg);
  void open_device();
  void init_format();
  void init_mmap();

private:
  V4l2Config config_;
  int fd_;
  Buffer *buffers_;
  uint32_t buffer_count_;
  bool is_streaming_;
};

#endif // YUYV_PROCESSOR_H
// ==========================================================
//    src/cpp/video/formats/v4l2_capture.h (Factory Pattern版)
// ==========================================================
#ifndef V4L2_CAPTURE_H
#define V4L2_CAPTURE_H

#include "capture_interface.h"
#include "config/config_manager.h"
#include <atomic>
#include <cstdint>
#include <string>

// *** 实现 ICapture 接口的 V4L2 捕获器 ***
class V4l2Capture : public ICapture {
public:
  explicit V4l2Capture(const V4l2Config &config);
  ~V4l2Capture();

  // 实现 ICapture 接口
  void start() override;
  void stop() override;
  bool capture(CapturedFrame &out_frame, std::atomic<bool> &running) override;

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

  // 当前帧数据，用于实现 capture() 方法
  CapturedFrame current_frame_;
};

#endif // V4L2_CAPTURE_H
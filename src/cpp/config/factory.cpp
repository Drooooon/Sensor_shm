#include "factory.h"
#include "video/formats/mjpg_decoder.h"
#include "video/formats/v4l2_capture.h"
#include "video/formats/yuyv_decoder.h"

std::unique_ptr<ICapture> Factory::create_capture(const V4l2Config &config) {
  // 目前我们只有 V4l2Capture，未来可以根据配置选择不同的捕获器
  return std::make_unique<V4l2Capture>(config);
}

std::unique_ptr<IDecoder> Factory::create_decoder(ImageFormat format) {
  switch (format) {
  case ImageFormat::YUYV:
    return std::make_unique<YuyvDecoder>();
  case ImageFormat::MJPG:
    return std::make_unique<MjpgDecoder>();
  // case ImageFormat::H264: ...
  default:
    return nullptr;
  }
}
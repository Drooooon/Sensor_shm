#ifndef DECODER_INTERFACE_H
#define DECODER_INTERFACE_H

#include "video/image_shm_manager.h"
#include <opencv2/opencv.hpp>

class IDecoder {
public:
  virtual ~IDecoder() = default;
  // 将从共享内存读取的数据解码为 BGR Mat
  virtual cv::Mat decode(const uint8_t *data, const ImageHeader &header) = 0;
};

#endif // DECODER_INTERFACE_H
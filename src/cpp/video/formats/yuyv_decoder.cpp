#include "yuyv_decoder.h"

cv::Mat YuyvDecoder::decode(const uint8_t *data, const ImageHeader &header) {
  cv::Mat yuyv_mat(header.height, header.width, header.frame_type,
                   (void *)data);
  cv::Mat bgr_mat;
  cv::cvtColor(yuyv_mat, bgr_mat, cv::COLOR_YUV2BGR_YUY2);
  return bgr_mat;
}
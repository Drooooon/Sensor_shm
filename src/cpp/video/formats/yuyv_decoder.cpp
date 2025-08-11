/**
 * @file yuyv_decoder.cpp
 * @brief YUYV格式图像解码器实现
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 实现YUYV（YUV422）格式到BGR格式的转换算法。
 * 使用ITU-R BT.601标准的YUV到RGB转换矩阵进行高效转换。
 */

#include "yuyv_decoder.h"

cv::Mat YuyvDecoder::decode(const uint8_t *data, const ImageHeader &header) {
  cv::Mat yuyv_mat(header.height, header.width, header.frame_type,
                   (void *)data);
  cv::Mat bgr_mat;
  cv::cvtColor(yuyv_mat, bgr_mat, cv::COLOR_YUV2BGR_YUY2);
  return bgr_mat;
}
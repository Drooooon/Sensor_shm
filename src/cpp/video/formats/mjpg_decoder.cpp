/**
 * @file mjpg_decoder.cpp
 * @brief MJPEG格式图像解码器实现
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 实现MJPEG（Motion JPEG）格式的解码功能，利用OpenCV内置的
 * JPEG解码器提供高效的图像解压缩服务。
 */

#include "mjpg_decoder.h"

cv::Mat MjpgDecoder::decode(const uint8_t *data, const ImageHeader &header) {
  cv::Mat compressed_mat(1, header.data_size, CV_8UC1, (void *)data);
  return cv::imdecode(compressed_mat, cv::IMREAD_COLOR);
}
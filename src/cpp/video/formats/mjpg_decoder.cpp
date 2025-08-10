#include "mjpg_decoder.h"

cv::Mat MjpgDecoder::decode(const uint8_t* data, const ImageHeader& header) {
    cv::Mat compressed_mat(1, header.data_size, CV_8UC1, (void*)data);
    return cv::imdecode(compressed_mat, cv::IMREAD_COLOR);
}
#ifndef YUYV_DECODER_H
#define YUYV_DECODER_H

#include "decoder_interface.h"

class YuyvDecoder : public IDecoder {
public:
  cv::Mat decode(const uint8_t *data, const ImageHeader &header) override;
};

#endif // YUYV_DECODER_H
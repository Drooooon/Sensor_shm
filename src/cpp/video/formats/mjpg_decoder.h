#ifndef MJPG_DECODER_H
#define MJPG_DECODER_H

#include "decoder_interface.h"

class MjpgDecoder : public IDecoder {
public:
  cv::Mat decode(const uint8_t *data, const ImageHeader &header) override;
};

#endif // MJPG_DECODER_H
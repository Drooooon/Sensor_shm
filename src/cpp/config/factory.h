#ifndef FACTORY_H
#define FACTORY_H

#include "video/formats/capture_interface.h"
#include "video/formats/decoder_interface.h"
#include "video/image_shm_manager.h"
#include <memory>

class Factory {
public:
  static std::unique_ptr<ICapture> create_capture(const V4l2Config &config);
  static std::unique_ptr<IDecoder> create_decoder(ImageFormat format);
};

#endif // FACTORY_H
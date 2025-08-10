#ifndef IMAGE_SHM_MANAGER_H
#define IMAGE_SHM_MANAGER_H

#include "common/ipc/shm_manager.h"
#include <cstdint>
#include <string>

enum class ImageFormat { YUYV, H264, BGR, MJPG };

struct ImageHeader {
  ImageFormat format;
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  uint32_t data_size;
  uint8_t frame_type;
};

class ImageShmManager : public ShmManager {
public:
  ImageShmManager(const std::string &shm_name) : ShmManager(shm_name) {}

  ShmStatus write_image(const uint8_t *image_data, size_t image_data_size,
                        uint32_t width, uint32_t height, uint32_t channels,
                        uint64_t frame_version, ImageFormat format,
                        uint8_t frame_type = 0);

  ShmStatus read_image(uint8_t *out_buffer, size_t max_buffer_size,
                       uint32_t *out_width, uint32_t *out_height,
                       uint32_t *out_channels, size_t *out_data_size,
                       uint64_t *out_frame_version, uint64_t *out_timestamp_us,
                       ImageFormat *out_format, uint8_t *out_frame_type);

private:
  static constexpr size_t HEADER_SIZE = sizeof(ImageHeader);
};

#endif // IMAGE_SHM_MANAGER_H
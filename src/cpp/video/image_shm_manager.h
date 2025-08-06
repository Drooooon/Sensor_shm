#ifndef IMAGE_SHM_MANAGER_H
#define IMAGE_SHM_MANAGER_H

#include "../common/shm_manager.h"
#include <cstdint>
#include <string>

enum class ImageFormat { YUYV, H264 };

struct ImageHeader {
  ImageFormat format; // YUYV或H.264
  uint32_t width;     // 图像宽度
  uint32_t height;    // 图像高度
  uint32_t channels;  // YUYV=2（YUV422），H.264忽略
  uint32_t data_size; // 数据字节数
  uint64_t timestamp; // 微秒时间戳
  uint8_t frame_type; // H.264用（1=I帧，2=P帧，3=B帧），YUYV忽略
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
                       uint64_t *out_frame_version, ImageFormat *out_format,
                       uint8_t *out_frame_type);

private:
  // 图像数据在缓冲区中的布局：[ImageHeader][图像数据]
  static constexpr size_t HEADER_SIZE = sizeof(ImageHeader);
};

#endif // IMAGE_SHM_MANAGER_H
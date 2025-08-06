#include "image_shm_manager.h"
#include <chrono>
#include <cstring>

ShmStatus ImageShmManager::write_image(const uint8_t *image_data,
                                       size_t image_data_size, uint32_t width,
                                       uint32_t height, uint32_t channels,
                                       uint64_t frame_version,
                                       ImageFormat format, uint8_t frame_type) {
  if (!image_data || image_data_size == 0 || width == 0 || height == 0 ||
      (format == ImageFormat::YUYV && channels != 2)) {
    return ShmStatus::InvalidArguments;
  }

  // 检查缓冲区大小是否足够容纳头部和数据
  size_t total_size = HEADER_SIZE + image_data_size;
  if (total_size > get_buffer_size()) {
    return ShmStatus::BufferTooSmall;
  }

  WriteBufferGuard guard = acquire_write_buffer(total_size);
  if (!guard.is_valid()) {
    return ShmStatus::BufferInUse;
  }

  // 准备图像头信息
  ImageHeader header = {
      format,
      width,
      height,
      channels,
      static_cast<uint32_t>(image_data_size),
      static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count()),
      frame_type};

  // 在缓冲区开头写入头部信息
  uint8_t *buffer_ptr = static_cast<uint8_t *>(guard.get());
  std::memcpy(buffer_ptr, &header, HEADER_SIZE);

  // 在头部后面写入图像数据
  std::memcpy(buffer_ptr + HEADER_SIZE, image_data, image_data_size);

  return guard.commit(total_size, frame_version);
}

ShmStatus
ImageShmManager::read_image(uint8_t *out_buffer, size_t max_buffer_size,
                            uint32_t *out_width, uint32_t *out_height,
                            uint32_t *out_channels, size_t *out_data_size,
                            uint64_t *out_frame_version,
                            ImageFormat *out_format, uint8_t *out_frame_type) {
  ReadBufferGuard guard = acquire_read_buffer();
  if (!guard.is_valid()) {
    return ShmStatus::NoDataAvailable;
  }

  // 检查缓冲区大小是否至少包含头部
  if (guard.size() < HEADER_SIZE) {
    return ShmStatus::InvalidArguments;
  }

  // 从缓冲区开头读取头部信息
  const uint8_t *buffer_ptr = static_cast<const uint8_t *>(guard.get());
  ImageHeader header;
  std::memcpy(&header, buffer_ptr, HEADER_SIZE);

  // 验证数据完整性
  if (guard.size() < HEADER_SIZE + header.data_size) {
    return ShmStatus::InvalidArguments;
  }

  if (header.data_size > max_buffer_size) {
    return ShmStatus::BufferTooSmall;
  }

  // 复制图像数据（跳过头部）
  std::memcpy(out_buffer, buffer_ptr + HEADER_SIZE, header.data_size);

  // 填充输出参数
  if (out_width)
    *out_width = header.width;
  if (out_height)
    *out_height = header.height;
  if (out_channels)
    *out_channels = header.channels;
  if (out_data_size)
    *out_data_size = header.data_size;
  if (out_frame_version)
    *out_frame_version = guard.frame_version();
  if (out_format)
    *out_format = header.format;
  if (out_frame_type)
    *out_frame_type = header.frame_type;

  return ShmStatus::Success;
}
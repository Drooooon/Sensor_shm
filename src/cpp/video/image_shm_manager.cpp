#include "image_shm_manager.h"
#include <chrono>
#include <cstring>
#include <iostream>

ShmStatus ImageShmManager::write_image(const uint8_t *image_data,
                                       size_t image_data_size, uint32_t width,
                                       uint32_t height, uint32_t channels,
                                       uint64_t frame_version,
                                       ImageFormat format, uint8_t frame_type) {

  if (!image_data || image_data_size == 0) {
    return ShmStatus::InvalidArguments;
  }

  size_t total_size = HEADER_SIZE + image_data_size;
  if (total_size > get_buffer_size()) {
    return ShmStatus::BufferTooSmall;
  }

  WriteBufferGuard guard = acquire_write_buffer(total_size);
  if (!guard.is_valid()) {
    return ShmStatus::BufferInUse;
  }

  ImageHeader header = {format,
                        width,
                        height,
                        channels,
                        static_cast<uint32_t>(image_data_size),
                        frame_type};

  uint8_t *buffer_ptr = static_cast<uint8_t *>(guard.get());
  if (!buffer_ptr) {
    // 这是一个不太可能发生的严重错误，但最好检查一下
    return ShmStatus::InvalidArguments;
  }

  std::memcpy(buffer_ptr, &header, HEADER_SIZE);
  std::memcpy(buffer_ptr + HEADER_SIZE, image_data, image_data_size);

  uint64_t current_timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  return guard.commit(total_size, frame_version, current_timestamp);
}
ShmStatus ImageShmManager::read_image(
    uint8_t *out_buffer, size_t max_buffer_size, uint32_t *out_width,
    uint32_t *out_height, uint32_t *out_channels, size_t *out_data_size,
    uint64_t *out_frame_version, uint64_t *out_timestamp_us,
    ImageFormat *out_format, uint8_t *out_frame_type) {
  ReadBufferGuard guard = acquire_read_buffer();
  if (!guard.is_valid())
    return ShmStatus::NoDataAvailable;

  if (guard.size() < HEADER_SIZE)
    return ShmStatus::InvalidArguments;

  const uint8_t *buffer_ptr = static_cast<const uint8_t *>(guard.get());
  ImageHeader header;
  std::memcpy(&header, buffer_ptr, HEADER_SIZE);

  if (guard.size() != HEADER_SIZE + header.data_size)
    return ShmStatus::InvalidArguments;
  if (header.data_size > max_buffer_size)
    return ShmStatus::BufferTooSmall;

  std::memcpy(out_buffer, buffer_ptr + HEADER_SIZE, header.data_size);

  if (out_width)
    *out_width = header.width;
  if (out_height)
    *out_height = header.height;
  if (out_channels)
    *out_channels = header.channels;
  if (out_data_size)
    *out_data_size = header.data_size;
  if (out_format)
    *out_format = header.format;
  if (out_frame_type)
    *out_frame_type = header.frame_type;
  if (out_frame_version)
    *out_frame_version = guard.frame_version();
  if (out_timestamp_us)
    *out_timestamp_us = guard.timestamp_us();

  return ShmStatus::Success;
}
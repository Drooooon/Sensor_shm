/**
 * @file shm_types.h
 * @author Dron
 * @brief 共享内存数据结构定义
 * @version 0.5
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef SHM_MANAGER_SHM_TYPES_H
#define SHM_MANAGER_SHM_TYPES_H

#include <atomic>
#include <cstddef>
#include <cstdint>

constexpr uint32_t NUM_BUFFERS = 3;

enum class ShmStatus {
  Success = 0,
  AlreadyInitialized,
  NotInitialized,
  ShmOpenFailed,
  ShmTruncateFailed,
  ShmMapFailed,
  ShmUnmapFailed,
  ShmUnlinkFailed,
  InvalidArguments,
  BufferTooSmall,
  BufferInUse,
  NoDataAvailable,
  AcquireFailed
};

enum class ShmState { Uninitialized, Created, Mapped, Closed };

struct ShmBufferControl {
  std::atomic<uint64_t> frame_version[NUM_BUFFERS];
  std::atomic<uint64_t> timestamp_us[NUM_BUFFERS]; // <--- 新增
  std::atomic<size_t> buffer_data_size[NUM_BUFFERS];
  std::atomic<bool> buffer_ready[NUM_BUFFERS];
  std::atomic<uint32_t> buffer_reader_count[NUM_BUFFERS];

  void initialize() {
    for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
      frame_version[i].store(0, std::memory_order_release);
      timestamp_us[i].store(0, std::memory_order_release); // <--- 新增
      buffer_data_size[i].store(0, std::memory_order_release);
      buffer_ready[i].store(false, std::memory_order_release);
      buffer_reader_count[i].store(0, std::memory_order_release);
    }
  }

  // 辅助函数，用于获取缓冲区数据大小
  size_t get_buffer_data_size(uint32_t buffer_idx) const {
    if (buffer_idx >= NUM_BUFFERS)
      return 0;
    return buffer_data_size[buffer_idx].load(std::memory_order_acquire);
  }

  // 辅助函数，用于获取缓冲区帧版本
  uint64_t get_frame_version(uint32_t buffer_idx) const {
    if (buffer_idx >= NUM_BUFFERS)
      return 0;
    return frame_version[buffer_idx].load(std::memory_order_acquire);
  }

  // 辅助函数，用于获取缓冲区时间戳
  uint64_t get_timestamp_us(uint32_t buffer_idx) const {
    if (buffer_idx >= NUM_BUFFERS)
      return 0;
    return timestamp_us[buffer_idx].load(std::memory_order_acquire);
  }
};

class ShmManager;

class WriteBufferGuard {
public:
  WriteBufferGuard(ShmManager *manager, size_t expected_size);
  ~WriteBufferGuard();
  ShmStatus commit(size_t actual_size, uint64_t frame_version,
                   uint64_t timestamp_us);
  void *get() { return buffer_; }
  const void *get() const { return buffer_; }
  size_t capacity() const { return capacity_; }
  bool is_valid() const { return buffer_ != nullptr; }

private:
  ShmManager *manager_;
  void *buffer_;
  size_t capacity_;
  uint32_t buffer_idx_;
  bool committed_;
};

class ReadBufferGuard {
public:
  ReadBufferGuard(ShmManager *manager);
  ~ReadBufferGuard();
  const void *get() const { return buffer_; }
  size_t size() const { return size_; }
  uint64_t frame_version() const { return frame_version_; }
  uint64_t timestamp_us() const { return timestamp_us_; } // <--- 新增
  bool is_valid() const { return buffer_ != nullptr; }
  ShmStatus status() const { return status_; }

private:
  ShmManager *manager_;
  const void *buffer_;
  size_t size_;
  uint64_t frame_version_;
  uint64_t timestamp_us_; // <--- 新增
  uint32_t buffer_idx_;
  ShmStatus status_;
};

#endif // SHM_MANAGER_SHM_TYPES_H
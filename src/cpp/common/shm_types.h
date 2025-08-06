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
#include <cstdint>
#include <cstddef>

// 缓冲区数量
constexpr uint32_t NUM_BUFFERS = 3;

// ShmStatus 枚举类型
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
  AcquireFailed,
};

// ShmState 枚举类型
enum class ShmState { Uninitialized, Created, Mapped, Closed };

// 共享内存缓冲区控制块
struct ShmBufferControl {
  std::atomic<uint64_t> frame_version[NUM_BUFFERS];
  std::atomic<size_t> buffer_data_size[NUM_BUFFERS];
  std::atomic<bool> buffer_ready[NUM_BUFFERS];
  std::atomic<uint32_t> buffer_reader_count[NUM_BUFFERS];

  void initialize() {
    for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
      frame_version[i].store(0, std::memory_order_release);
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
};

class ShmManager; // 前向声明

// 零拷贝 RAII 包装器，用于写入缓冲区
class WriteBufferGuard {
public:
  // 构造函数会尝试获取一个可写的缓冲区
  WriteBufferGuard(ShmManager *manager, size_t expected_size);
  // 析构函数会自动释放未提交的缓冲区
  ~WriteBufferGuard();

  // 提交写入的数据，并更新帧版本和数据大小
  ShmStatus commit(size_t actual_size, uint64_t frame_version);

  // 获取缓冲区指针
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

// 零拷贝 RAII 包装器，用于读取缓冲区
class ReadBufferGuard {
public:
  // 构造函数会尝试获取一个最新的可读缓冲区
  ReadBufferGuard(ShmManager *manager);
  // 析构函数会自动释放读缓冲区
  ~ReadBufferGuard();

  // 获取缓冲区指针
  const void *get() const { return buffer_; }
  // 获取数据大小
  size_t size() const { return size_; }
  // 获取帧版本
  uint64_t frame_version() const { return frame_version_; }
  // 检查是否成功获取缓冲区
  bool is_valid() const { return buffer_ != nullptr; }
  // 获取状态码
  ShmStatus status() const { return status_; }

private:
  ShmManager *manager_;
  const void *buffer_;
  size_t size_;
  uint64_t frame_version_;
  uint32_t buffer_idx_;
  ShmStatus status_;
};

#endif // SHM_MANAGER_SHM_TYPES_H
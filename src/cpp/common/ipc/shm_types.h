/**
 * @file shm_types.h
 * @author Dron
 * @brief 共享内存数据结构定义
 * @version 0.6
 * @date 2025-08-10
 *
 * 该文件定义了共享内存管理所需的所有数据结构、枚举类型和RAII守卫类。
 * 包括状态码定义、缓冲区控制结构以及内存安全的访问接口。
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef SHM_MANAGER_SHM_TYPES_H
#define SHM_MANAGER_SHM_TYPES_H

#include <atomic>
#include <cstddef>
#include <cstdint>

/**
 * @brief 共享内存操作状态码枚举
 *
 * 定义了所有可能的共享内存操作结果状态，用于错误处理和状态检查。
 */
enum class ShmStatus {
  Success = 0,        ///< 操作成功
  AlreadyInitialized, ///< 已经初始化
  NotInitialized,     ///< 尚未初始化
  ShmOpenFailed,      ///< 共享内存打开失败
  ShmTruncateFailed,  ///< 共享内存截断失败
  ShmMapFailed,       ///< 内存映射失败
  ShmUnmapFailed,     ///< 内存解除映射失败
  ShmUnlinkFailed,    ///< 共享内存删除失败
  InvalidArguments,   ///< 无效参数
  BufferTooSmall,     ///< 缓冲区太小
  BufferInUse,        ///< 缓冲区正在使用
  NoDataAvailable,    ///< 没有可用数据
  AcquireFailed       ///< 获取缓冲区失败
};

/**
 * @brief 共享内存状态枚举
 *
 * 描述共享内存的当前生命周期状态。
 */
enum class ShmState {
  Uninitialized, ///< 未初始化状态
  Created,       ///< 已创建状态
  Mapped,        ///< 已映射状态
  Closed         ///< 已关闭状态
};

/**
 * @brief 共享内存缓冲区控制结构
 *
 * 管理多个缓冲区的元数据，支持动态缓冲区数量配置。
 * 在共享内存中的布局：
 * [ShmBufferControl头部] [frame_version数组] [timestamp_us数组]
 * [buffer_data_size数组] [buffer_ready数组] [buffer_reader_count数组]
 * [数据缓冲区...]
 *
 * 使用原子操作确保多进程访问的线程安全性。
 */
struct ShmBufferControl {
  std::atomic<uint32_t> buffer_count; ///< 缓冲区数量
  size_t buffer_size;                 ///< 单个缓冲区大小

  /**
   * @brief 获取帧版本数组在共享内存中的偏移量
   * @return size_t 偏移量（字节）
   */
  static size_t get_frame_version_offset() { return sizeof(ShmBufferControl); }

  /**
   * @brief 获取时间戳数组在共享内存中的偏移量
   * @param buffer_count 缓冲区数量
   * @return size_t 偏移量（字节）
   */
  static size_t get_timestamp_us_offset(uint32_t buffer_count) {
    return get_frame_version_offset() +
           buffer_count * sizeof(std::atomic<uint64_t>);
  }

  /**
   * @brief 获取缓冲区数据大小数组的偏移量
   * @param buffer_count 缓冲区数量
   * @return size_t 偏移量（字节）
   */
  static size_t get_buffer_data_size_offset(uint32_t buffer_count) {
    return get_timestamp_us_offset(buffer_count) +
           buffer_count * sizeof(std::atomic<uint64_t>);
  }

  /**
   * @brief 获取缓冲区就绪标志数组的偏移量
   * @param buffer_count 缓冲区数量
   * @return size_t 偏移量（字节）
   */
  static size_t get_buffer_ready_offset(uint32_t buffer_count) {
    return get_buffer_data_size_offset(buffer_count) +
           buffer_count * sizeof(std::atomic<size_t>);
  }

  /**
   * @brief 获取读者计数数组的偏移量
   * @param buffer_count 缓冲区数量
   * @return size_t 偏移量（字节）
   */
  static size_t get_buffer_reader_count_offset(uint32_t buffer_count) {
    return get_buffer_ready_offset(buffer_count) +
           buffer_count * sizeof(std::atomic<bool>);
  }

  /**
   * @brief 获取数据缓冲区区域的起始偏移量
   * @param buffer_count 缓冲区数量
   * @return size_t 偏移量（字节）
   */
  static size_t get_data_buffers_offset(uint32_t buffer_count) {
    return get_buffer_reader_count_offset(buffer_count) +
           buffer_count * sizeof(std::atomic<uint32_t>);
  }

  /**
   * @brief 初始化缓冲区控制结构
   * @param num_buffers 缓冲区数量
   * @param single_buffer_size 单个缓冲区大小
   * @param base_ptr 共享内存基地址
   */
  void initialize(uint32_t num_buffers, size_t single_buffer_size,
                  void *base_ptr) {
    buffer_count.store(num_buffers, std::memory_order_release);
    buffer_size = single_buffer_size;

    char *base = static_cast<char *>(base_ptr);

    // 获取各数组的指针
    auto *frame_version = reinterpret_cast<std::atomic<uint64_t> *>(
        base + get_frame_version_offset());
    auto *timestamp_us = reinterpret_cast<std::atomic<uint64_t> *>(
        base + get_timestamp_us_offset(num_buffers));
    auto *buffer_data_size = reinterpret_cast<std::atomic<size_t> *>(
        base + get_buffer_data_size_offset(num_buffers));
    auto *buffer_ready = reinterpret_cast<std::atomic<bool> *>(
        base + get_buffer_ready_offset(num_buffers));
    auto *buffer_reader_count = reinterpret_cast<std::atomic<uint32_t> *>(
        base + get_buffer_reader_count_offset(num_buffers));

    for (uint32_t i = 0; i < num_buffers; ++i) {
      new (&frame_version[i]) std::atomic<uint64_t>(0);
      new (&timestamp_us[i]) std::atomic<uint64_t>(0);
      new (&buffer_data_size[i]) std::atomic<size_t>(0);
      new (&buffer_ready[i]) std::atomic<bool>(false);
      new (&buffer_reader_count[i]) std::atomic<uint32_t>(0);
    }
  }

  /**
   * @brief 获取指定缓冲区的数据大小
   * @param buffer_idx 缓冲区索引
   * @param base_ptr 共享内存基地址
   * @return size_t 数据大小，无效索引时返回0
   */
  size_t get_buffer_data_size(uint32_t buffer_idx, void *base_ptr) const {
    uint32_t num_buffers = buffer_count.load(std::memory_order_acquire);
    if (buffer_idx >= num_buffers)
      return 0;

    char *base = static_cast<char *>(base_ptr);
    auto *buffer_data_size = reinterpret_cast<std::atomic<size_t> *>(
        base + get_buffer_data_size_offset(num_buffers));
    return buffer_data_size[buffer_idx].load(std::memory_order_acquire);
  }

  /**
   * @brief 获取指定缓冲区的帧版本号
   * @param buffer_idx 缓冲区索引
   * @param base_ptr 共享内存基地址
   * @return uint64_t 帧版本号，无效索引时返回0
   */
  uint64_t get_frame_version(uint32_t buffer_idx, void *base_ptr) const {
    uint32_t num_buffers = buffer_count.load(std::memory_order_acquire);
    if (buffer_idx >= num_buffers)
      return 0;

    char *base = static_cast<char *>(base_ptr);
    auto *frame_version = reinterpret_cast<std::atomic<uint64_t> *>(
        base + get_frame_version_offset());
    return frame_version[buffer_idx].load(std::memory_order_acquire);
  }

  /**
   * @brief 获取指定缓冲区的时间戳
   * @param buffer_idx 缓冲区索引
   * @param base_ptr 共享内存基地址
   * @return uint64_t 时间戳（微秒），无效索引时返回0
   */
  uint64_t get_timestamp_us(uint32_t buffer_idx, void *base_ptr) const {
    uint32_t num_buffers = buffer_count.load(std::memory_order_acquire);
    if (buffer_idx >= num_buffers)
      return 0;

    char *base = static_cast<char *>(base_ptr);
    auto *timestamp_us = reinterpret_cast<std::atomic<uint64_t> *>(
        base + get_timestamp_us_offset(num_buffers));
    return timestamp_us[buffer_idx].load(std::memory_order_acquire);
  }

  /**
   * @brief 获取帧版本数组指针
   * @param base_ptr 共享内存基地址
   * @return std::atomic<uint64_t>* 帧版本数组指针
   */
  std::atomic<uint64_t> *get_frame_version_array(void *base_ptr) const {
    char *base = static_cast<char *>(base_ptr);
    return reinterpret_cast<std::atomic<uint64_t> *>(
        base + get_frame_version_offset());
  }

  /**
   * @brief 获取时间戳数组指针
   * @param base_ptr 共享内存基地址
   * @return std::atomic<uint64_t>* 时间戳数组指针
   */
  std::atomic<uint64_t> *get_timestamp_us_array(void *base_ptr) const {
    uint32_t num_buffers = buffer_count.load(std::memory_order_acquire);
    char *base = static_cast<char *>(base_ptr);
    return reinterpret_cast<std::atomic<uint64_t> *>(
        base + get_timestamp_us_offset(num_buffers));
  }

  /**
   * @brief 获取缓冲区数据大小数组指针
   * @param base_ptr 共享内存基地址
   * @return std::atomic<size_t>* 数据大小数组指针
   */
  std::atomic<size_t> *get_buffer_data_size_array(void *base_ptr) const {
    uint32_t num_buffers = buffer_count.load(std::memory_order_acquire);
    char *base = static_cast<char *>(base_ptr);
    return reinterpret_cast<std::atomic<size_t> *>(
        base + get_buffer_data_size_offset(num_buffers));
  }

  /**
   * @brief 获取缓冲区就绪标志数组指针
   * @param base_ptr 共享内存基地址
   * @return std::atomic<bool>* 就绪标志数组指针
   */
  std::atomic<bool> *get_buffer_ready_array(void *base_ptr) const {
    uint32_t num_buffers = buffer_count.load(std::memory_order_acquire);
    char *base = static_cast<char *>(base_ptr);
    return reinterpret_cast<std::atomic<bool> *>(
        base + get_buffer_ready_offset(num_buffers));
  }

  /**
   * @brief 获取读者计数数组指针
   * @param base_ptr 共享内存基地址
   * @return std::atomic<uint32_t>* 读者计数数组指针
   */
  std::atomic<uint32_t> *get_buffer_reader_count_array(void *base_ptr) const {
    uint32_t num_buffers = buffer_count.load(std::memory_order_acquire);
    char *base = static_cast<char *>(base_ptr);
    return reinterpret_cast<std::atomic<uint32_t> *>(
        base + get_buffer_reader_count_offset(num_buffers));
  }
};

// 前向声明
class ShmManager;

/**
 * @brief 写缓冲区RAII守卫类
 *
 * 提供内存安全的写缓冲区访问，自动管理缓冲区的获取和释放。
 * 使用RAII模式确保在异常情况下也能正确释放资源。
 */
class WriteBufferGuard {
public:
  /**
   * @brief 构造函数，获取写缓冲区
   * @param manager 共享内存管理器指针
   * @param expected_size 预期写入的数据大小
   */
  WriteBufferGuard(ShmManager *manager, size_t expected_size);

  /**
   * @brief 析构函数，自动释放缓冲区资源
   */
  ~WriteBufferGuard();

  /**
   * @brief 提交写入的数据
   * @param actual_size 实际写入的数据大小
   * @param frame_version 帧版本号
   * @param timestamp_us 时间戳（微秒）
   * @return ShmStatus 提交结果状态码
   */
  ShmStatus commit(size_t actual_size, uint64_t frame_version,
                   uint64_t timestamp_us);

  /**
   * @brief 获取缓冲区指针
   * @return void* 可写缓冲区指针
   */
  void *get() { return buffer_; }

  /**
   * @brief 获取缓冲区指针（常量版本）
   * @return const void* 只读缓冲区指针
   */
  const void *get() const { return buffer_; }

  /**
   * @brief 获取缓冲区容量
   * @return size_t 缓冲区容量（字节）
   */
  size_t capacity() const { return capacity_; }

  /**
   * @brief 检查缓冲区是否有效
   * @return bool true表示有效，false表示无效
   */
  bool is_valid() const { return buffer_ != nullptr; }

private:
  ShmManager *manager_; ///< 管理器指针
  void *buffer_;        ///< 缓冲区指针
  size_t capacity_;     ///< 缓冲区容量
  uint32_t buffer_idx_; ///< 缓冲区索引
  bool committed_;      ///< 是否已提交标志
};

/**
 * @brief 读缓冲区RAII守卫类
 *
 * 提供内存安全的读缓冲区访问，自动管理缓冲区的获取和释放。
 * 支持读取数据的同时获取元数据（帧版本、时间戳等）。
 */
class ReadBufferGuard {
public:
  /**
   * @brief 构造函数，获取读缓冲区
   * @param manager 共享内存管理器指针
   */
  ReadBufferGuard(ShmManager *manager);

  /**
   * @brief 析构函数，自动释放缓冲区资源
   */
  ~ReadBufferGuard();

  /**
   * @brief 获取数据指针
   * @return const void* 只读数据指针
   */
  const void *get() const { return buffer_; }

  /**
   * @brief 获取数据大小
   * @return size_t 数据大小（字节）
   */
  size_t size() const { return size_; }

  /**
   * @brief 获取帧版本号
   * @return uint64_t 帧版本号
   */
  uint64_t frame_version() const { return frame_version_; }

  /**
   * @brief 获取时间戳
   * @return uint64_t 时间戳（微秒）
   */
  uint64_t timestamp_us() const { return timestamp_us_; }

  /**
   * @brief 检查缓冲区是否有效
   * @return bool true表示有效，false表示无效
   */
  bool is_valid() const { return buffer_ != nullptr; }

  /**
   * @brief 获取操作状态
   * @return ShmStatus 获取缓冲区的结果状态
   */
  ShmStatus status() const { return status_; }

private:
  ShmManager *manager_;    ///< 管理器指针
  const void *buffer_;     ///< 数据指针
  size_t size_;            ///< 数据大小
  uint64_t frame_version_; ///< 帧版本号
  uint64_t timestamp_us_;  ///< 时间戳
  uint32_t buffer_idx_;    ///< 缓冲区索引
  ShmStatus status_;       ///< 操作状态
};

#endif // SHM_MANAGER_SHM_TYPES_H
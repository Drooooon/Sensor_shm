/**
 * @file shm_manager.h
 * @author Dron
 * @brief 零拷贝共享内存管理器
 * @version 0.5
 * @date 2025-08-05
 *
 * 该文件实现了一个高性能的共享内存管理器，支持多进程间的零拷贝数据传输。
 * 提供了生产者-消费者模式的缓冲区管理，以及C++和C语言的双重接口。
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

#include "shm_types.h"
#include <atomic>
#include <mutex>
#include <string>

/**
 * @brief 将共享内存状态码转换为可读字符串
 * @param status 共享内存状态码
 * @return const char* 状态描述字符串
 */
const char *shm_status_to_string(ShmStatus status);

/**
 * @brief 零拷贝共享内存管理器类
 *
 * 该类提供了高效的多进程共享内存管理功能，支持：
 * - 零拷贝数据传输
 * - 多缓冲区环形队列
 * - 线程安全的读写操作
 * - 自动内存映射和清理
 * - RAII风格的缓冲区管理
 */
class ShmManager {
public:
  /**
   * @brief 构造函数
   * @param shm_name 共享内存名称，用于标识共享内存段
   */
  ShmManager(const std::string &shm_name);

  /**
   * @brief 析构函数，自动清理资源
   */
  ~ShmManager();

  /**
   * @brief 创建并初始化共享内存
   * @param shm_total_size 共享内存总大小（字节）
   * @param buffer_size 单个缓冲区大小（字节）
   * @param buffer_count 缓冲区数量
   * @return ShmStatus 操作结果状态码
   */
  ShmStatus create_and_init(size_t shm_total_size, size_t buffer_size,
                            uint32_t buffer_count);

  /**
   * @brief 打开并映射已存在的共享内存
   * @param shm_total_size 预期的共享内存总大小
   * @param buffer_size 预期的单个缓冲区大小
   * @param buffer_count 预期的缓冲区数量
   * @return ShmStatus 操作结果状态码
   */
  ShmStatus open_and_map(size_t shm_total_size, size_t buffer_size,
                         uint32_t buffer_count);

  /**
   * @brief 取消映射并关闭共享内存
   * @return ShmStatus 操作结果状态码
   */
  ShmStatus unmap_and_close();

  /**
   * @brief 删除共享内存段（仅创建者可调用）
   * @return ShmStatus 操作结果状态码
   */
  ShmStatus unlink_shm();

  /**
   * @brief 获取写缓冲区的RAII守卫对象
   * @param expected_size 预期写入的数据大小
   * @return WriteBufferGuard RAII风格的写缓冲区守卫
   */
  WriteBufferGuard acquire_write_buffer(size_t expected_size);

  /**
   * @brief 获取读缓冲区的RAII守卫对象
   * @return ReadBufferGuard RAII风格的读缓冲区守卫
   */
  ReadBufferGuard acquire_read_buffer();

  // 兼容性接口
  /**
   * @brief 写入数据并切换到下一个缓冲区（兼容接口）
   * @param data 要写入的数据指针
   * @param size 数据大小
   * @param frame_version 帧版本号
   * @return ShmStatus 操作结果状态码
   */
  ShmStatus write_and_switch(const void *data, size_t size,
                             uint64_t frame_version);

  /**
   * @brief 尝试读取最新数据（非阻塞）
   * @param data 接收数据的缓冲区
   * @param max_size 缓冲区最大容量
   * @param actual_size 实际读取的数据大小
   * @return ShmStatus 操作结果状态码
   */
  ShmStatus try_read_latest(void *data, size_t max_size, size_t *actual_size);

  /**
   * @brief 等待并读取数据（阻塞接口）
   * @param data 接收数据的缓冲区
   * @param max_size 缓冲区最大容量
   * @param actual_size 实际读取的数据大小
   * @return ShmStatus 操作结果状态码
   */
  ShmStatus wait_and_read(void *data, size_t max_size, size_t *actual_size);

  /**
   * @brief 获取共享内存指针
   * @return void* 共享内存的起始地址
   */
  void *get_shm_ptr() const;

  /**
   * @brief 获取共享内存大小
   * @return size_t 共享内存总大小（字节）
   */
  size_t get_shm_size() const;

  /**
   * @brief 获取单个缓冲区大小
   * @return size_t 单个缓冲区大小（字节）
   */
  size_t get_buffer_size() const;

  /**
   * @brief 获取当前状态
   * @return ShmState 当前共享内存状态
   */
  ShmState get_state() const;

  /**
   * @brief 检查是否已初始化
   * @return bool true表示已初始化，false表示未初始化
   */
  bool is_initialized() const;

  /**
   * @brief 获取指定缓冲区的帧版本号
   * @param buffer_idx 缓冲区索引
   * @return uint64_t 帧版本号
   */
  uint64_t get_frame_version(uint32_t buffer_idx) const;

private:
  friend class WriteBufferGuard;
  friend class ReadBufferGuard;

  // 内部接口，由友元类访问
  void *internal_acquire_write_buffer(size_t expected_size,
                                      uint32_t *buffer_idx);
  void internal_release_write_buffer(uint32_t buffer_idx);
  ShmStatus internal_commit_write_buffer(uint32_t buffer_idx,
                                         size_t actual_size,
                                         uint64_t frame_version,
                                         uint64_t timestamp_us);
  const void *internal_acquire_read_buffer(size_t *data_size,
                                           uint64_t *frame_version,
                                           uint64_t *timestamp_us,
                                           uint32_t *buffer_idx,
                                           ShmStatus *status);
  void internal_release_read_buffer(uint32_t buffer_idx);

  // 辅助方法
  void log_error(const std::string &message, ShmStatus status_code) const;
  void close_internal_handles();
  ShmStatus validate_buffer_layout(size_t shm_total_size, size_t buffer_size,
                                   uint32_t buffer_count) const;
  ShmBufferControl *get_buffer_control() const;
  void *get_data_buffer(uint32_t buffer_idx) const;
  void *get_data_buffer_nolock(uint32_t buffer_idx) const;

private:
  std::string shm_name_;                 ///< 共享内存名称
  int shm_fd_;                           ///< 共享内存文件描述符
  void *shm_ptr_;                        ///< 共享内存映射指针
  std::atomic<size_t> current_shm_size_; ///< 当前共享内存大小
  std::atomic<size_t> buffer_size_;      ///< 缓冲区大小
  ShmState state_;                       ///< 当前状态
  bool is_creator_;                      ///< 是否为创建者标志
  mutable std::mutex state_mutex_;       ///< 状态保护互斥锁
};

// ========== C接口声明 ==========
/**
 * @brief C语言接口，用于与其他语言绑定
 *
 * 以下函数提供了C语言兼容的共享内存管理接口，
 * 方便与Python、Go等语言进行FFI调用。
 */
extern "C" {

/**
 * @brief 创建共享内存管理器实例
 * @param shm_name 共享内存名称
 * @return void* 管理器实例指针，失败时返回NULL
 */
void *create_shm_manager(const char *shm_name);

/**
 * @brief 销毁共享内存管理器实例
 * @param manager_ptr 管理器实例指针
 */
void destroy_shm_manager(void *manager_ptr);

/**
 * @brief 创建并初始化共享内存
 * @param manager_ptr 管理器实例指针
 * @param shm_total_size 共享内存总大小（字节）
 * @param buffer_size 单个缓冲区大小（字节）
 * @param buffer_count 缓冲区数量
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_create_and_init(void *manager_ptr, size_t shm_total_size,
                                size_t buffer_size, uint32_t buffer_count);

/**
 * @brief 打开并映射已存在的共享内存
 * @param manager_ptr 管理器实例指针
 * @param shm_total_size 预期的共享内存总大小
 * @param buffer_size 预期的单个缓冲区大小
 * @param buffer_count 预期的缓冲区数量
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_open_and_map(void *manager_ptr, size_t shm_total_size,
                             size_t buffer_size, uint32_t buffer_count);

/**
 * @brief 取消映射并关闭共享内存
 * @param manager_ptr 管理器实例指针
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_unmap_and_close(void *manager_ptr);

/**
 * @brief 删除共享内存段（仅创建者可调用）
 * @param manager_ptr 管理器实例指针
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_unlink_shm(void *manager_ptr);

// 零拷贝C接口
/**
 * @brief 获取写缓冲区指针
 * @param manager_ptr 管理器实例指针
 * @param expected_size 预期写入的数据大小
 * @return void* 写缓冲区指针，失败时返回NULL
 */
void *shm_manager_acquire_write_buffer(void *manager_ptr, size_t expected_size);

/**
 * @brief 提交写入的缓冲区
 * @param manager_ptr 管理器实例指针
 * @param buffer_ptr 写缓冲区指针
 * @param actual_size 实际写入的数据大小
 * @param frame_version 帧版本号
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_commit_write_buffer(void *manager_ptr, void *buffer_ptr,
                                    size_t actual_size, uint64_t frame_version);

/**
 * @brief 释放写缓冲区
 * @param manager_ptr 管理器实例指针
 * @param buffer_ptr 写缓冲区指针
 */
void shm_manager_release_write_buffer(void *manager_ptr, void *buffer_ptr);

/**
 * @brief 获取读缓冲区数据
 * @param manager_ptr 管理器实例指针
 * @param data_size 输出参数，接收数据大小
 * @param frame_version 输出参数，接收帧版本号
 * @return const void* 读缓冲区数据指针，失败时返回NULL
 */
const void *shm_manager_acquire_read_buffer(void *manager_ptr,
                                            size_t *data_size,
                                            uint64_t *frame_version);

/**
 * @brief 阻塞等待数据到达并获取
 * @param manager_ptr 管理器实例指针
 * @param data_size 输出参数，接收数据大小
 * @param frame_version 输出参数，接收帧版本号
 * @return const void* 数据指针，失败时返回NULL
 */
const void *shm_manager_wait_for_data(void *manager_ptr, size_t *data_size,
                                      uint64_t *frame_version);

/**
 * @brief 释放读缓冲区
 * @param manager_ptr 管理器实例指针
 * @param buffer_ptr 读缓冲区指针
 */
void shm_manager_release_read_buffer(void *manager_ptr, const void *buffer_ptr);

// 兼容接口
/**
 * @brief 写入数据并切换到下一个缓冲区（兼容接口）
 * @param manager_ptr 管理器实例指针
 * @param data 要写入的数据指针
 * @param size 数据大小
 * @param frame_version 帧版本号
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_write_and_switch(void *manager_ptr, const void *data,
                                 size_t size, uint64_t frame_version);

/**
 * @brief 尝试读取最新数据（非阻塞）
 * @param manager_ptr 管理器实例指针
 * @param data 接收数据的缓冲区
 * @param max_size 缓冲区最大容量
 * @param actual_size 实际读取的数据大小
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_try_read_latest(void *manager_ptr, void *data, size_t max_size,
                                size_t *actual_size);

/**
 * @brief 等待并读取数据（阻塞接口）
 * @param manager_ptr 管理器实例指针
 * @param data 接收数据的缓冲区
 * @param max_size 缓冲区最大容量
 * @param actual_size 实际读取的数据大小
 * @return int 操作结果，0表示成功，其他值表示失败
 */
int shm_manager_wait_and_read(void *manager_ptr, void *data, size_t max_size,
                              size_t *actual_size);

/**
 * @brief 获取共享内存指针
 * @param manager_ptr 管理器实例指针
 * @return void* 共享内存的起始地址
 */
void *shm_manager_get_shm_ptr(const void *manager_ptr);

/**
 * @brief 获取共享内存大小
 * @param manager_ptr 管理器实例指针
 * @return size_t 共享内存总大小（字节）
 */
size_t shm_manager_get_shm_size(const void *manager_ptr);

/**
 * @brief 获取单个缓冲区大小
 * @param manager_ptr 管理器实例指针
 * @return size_t 单个缓冲区大小（字节）
 */
size_t shm_manager_get_buffer_size(const void *manager_ptr);

/**
 * @brief 获取指定缓冲区的帧版本号
 * @param manager_ptr 管理器实例指针
 * @param buffer_idx 缓冲区索引
 * @return uint64_t 帧版本号
 */
uint64_t shm_manager_get_frame_version(const void *manager_ptr,
                                       uint32_t buffer_idx);
}

#endif // SHM_MANAGER_H
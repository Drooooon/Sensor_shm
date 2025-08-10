/**
 * @file shm_manager.h
 * @author Dron
 * @brief 零拷贝共享内存管理器
 * @version 0.5
 * @date 2025-08-05
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

const char *shm_status_to_string(ShmStatus status);

class ShmManager
{
public:
  ShmManager(const std::string &shm_name);
  ~ShmManager();
  ShmStatus create_and_init(size_t shm_total_size, size_t buffer_size);
  ShmStatus open_and_map(size_t shm_total_size, size_t buffer_size);
  ShmStatus unmap_and_close();
  ShmStatus unlink_shm();

  WriteBufferGuard acquire_write_buffer(size_t expected_size);
  ReadBufferGuard acquire_read_buffer();

  // 兼容性接口
  ShmStatus write_and_switch(const void *data, size_t size,
                             uint64_t frame_version);
  ShmStatus try_read_latest(void *data, size_t max_size, size_t *actual_size);
  ShmStatus wait_and_read(void *data, size_t max_size, size_t *actual_size);

  void *get_shm_ptr() const;
  size_t get_shm_size() const;
  size_t get_buffer_size() const;
  ShmState get_state() const;
  bool is_initialized() const;
  uint64_t get_frame_version(uint32_t buffer_idx) const;

private:
  friend class WriteBufferGuard;
  friend class ReadBufferGuard;
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

  void log_error(const std::string &message, ShmStatus status_code) const;
  void close_internal_handles();
  ShmStatus validate_buffer_layout(size_t shm_total_size,
                                   size_t buffer_size) const;
  ShmBufferControl *get_buffer_control() const;
  void *get_data_buffer(uint32_t buffer_idx) const;
  void *get_data_buffer_nolock(uint32_t buffer_idx) const;

private:
  std::string shm_name_;
  int shm_fd_;
  void *shm_ptr_;
  std::atomic<size_t> current_shm_size_;
  std::atomic<size_t> buffer_size_;
  ShmState state_;
  bool is_creator_;
  mutable std::mutex state_mutex_;
};

// ========== C接口声明 ==========
extern "C" {
void *create_shm_manager(const char *shm_name);
void destroy_shm_manager(void *manager_ptr);
int shm_manager_create_and_init(void *manager_ptr, size_t shm_total_size,
                                size_t buffer_size);
int shm_manager_open_and_map(void *manager_ptr, size_t shm_total_size,
                             size_t buffer_size);
int shm_manager_unmap_and_close(void *manager_ptr);
int shm_manager_unlink_shm(void *manager_ptr);
// 零拷贝C接口
void *shm_manager_acquire_write_buffer(void *manager_ptr, size_t expected_size);
int shm_manager_commit_write_buffer(void *manager_ptr, void *buffer_ptr,
                                    size_t actual_size, uint64_t frame_version);
void shm_manager_release_write_buffer(void *manager_ptr, void *buffer_ptr);
const void *shm_manager_acquire_read_buffer(void *manager_ptr,
                                            size_t *data_size,
                                            uint64_t *frame_version);
const void *shm_manager_wait_for_data(void *manager_ptr, size_t *data_size,
                                      uint64_t *frame_version);
void shm_manager_release_read_buffer(void *manager_ptr, const void *buffer_ptr);
// 兼容接口
int shm_manager_write_and_switch(void *manager_ptr, const void *data,
                                 size_t size, uint64_t frame_version);
int shm_manager_try_read_latest(void *manager_ptr, void *data, size_t max_size,
                                size_t *actual_size);
int shm_manager_wait_and_read(void *manager_ptr, void *data, size_t max_size,
                              size_t *actual_size);
// 信息获取接口
void *shm_manager_get_shm_ptr(const void *manager_ptr);
size_t shm_manager_get_shm_size(const void *manager_ptr);
size_t shm_manager_get_buffer_size(const void *manager_ptr);
uint64_t shm_manager_get_frame_version(const void *manager_ptr,
                                       uint32_t buffer_idx);
}

#endif // SHM_MANAGER_H
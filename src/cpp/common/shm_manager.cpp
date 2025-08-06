/**
 * @file shm_manager.cpp
 * @author Dron
 * @brief 零拷贝共享内存管理器实现
 * @version 0.5
 * @date 2025-08-05
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "shm_manager.h"
#include <algorithm> // for std::min
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

// 移除重复定义，使用 shm_types.h 中的 NUM_BUFFERS

// 全局存储用于管理Guard对象的生命周期
static std::mutex guard_map_mutex;
static std::unordered_map<void *, std::unique_ptr<WriteBufferGuard>>
    write_guards;
static std::unordered_map<const void *, std::unique_ptr<ReadBufferGuard>>
    read_guards;

// ========== 状态码转换函数 ==========
const char *shm_status_to_string(ShmStatus status) {
  switch (status) {
  case ShmStatus::Success:
    return "Success";
  case ShmStatus::AlreadyInitialized:
    return "Already Initialized";
  case ShmStatus::NotInitialized:
    return "Not Initialized";
  case ShmStatus::ShmOpenFailed:
    return "Shared Memory Open Failed";
  case ShmStatus::ShmTruncateFailed:
    return "Shared Memory Truncate Failed";
  case ShmStatus::ShmMapFailed:
    return "Shared Memory Map Failed";
  case ShmStatus::ShmUnmapFailed:
    return "Shared Memory Unmap Failed";
  case ShmStatus::ShmUnlinkFailed:
    return "Shared Memory Unlink Failed";
  case ShmStatus::InvalidArguments:
    return "Invalid Arguments";
  case ShmStatus::BufferTooSmall:
    return "Buffer Too Small";
  case ShmStatus::BufferInUse:
    return "Buffer In Use";
  case ShmStatus::NoDataAvailable:
    return "No Data Available";
  case ShmStatus::AcquireFailed:
    return "Acquire Failed";
  default:
    return "Unknown Status";
  }
}

// ========== WriteBufferGuard Implementation ==========
WriteBufferGuard::WriteBufferGuard(ShmManager *manager, size_t expected_size)
    : manager_(manager), buffer_(nullptr), capacity_(0), buffer_idx_(0),
      committed_(false) {
  if (manager_) {
    buffer_ =
        manager_->internal_acquire_write_buffer(expected_size, &buffer_idx_);
    capacity_ = expected_size;
  }
}

WriteBufferGuard::~WriteBufferGuard() {
  if (buffer_ && !committed_ && manager_) {
    manager_->internal_release_write_buffer(buffer_idx_);
  }
}

ShmStatus WriteBufferGuard::commit(size_t actual_size, uint64_t frame_version) {
  if (!buffer_ || committed_ || !manager_) {
    return ShmStatus::InvalidArguments;
  }
  if (actual_size > capacity_) {
    return ShmStatus::BufferTooSmall;
  }
  committed_ = true;
  return manager_->internal_commit_write_buffer(buffer_idx_, actual_size,
                                                frame_version);
}

// ========== ReadBufferGuard Implementation ==========
ReadBufferGuard::ReadBufferGuard(ShmManager *manager)
    : manager_(manager), buffer_(nullptr), size_(0), frame_version_(0),
      buffer_idx_(0), status_(ShmStatus::Success) {
  if (manager_) {
    buffer_ = manager_->internal_acquire_read_buffer(&size_, &frame_version_,
                                                     &buffer_idx_, &status_);
  }
}

ReadBufferGuard::~ReadBufferGuard() {
  if (buffer_ && manager_) {
    manager_->internal_release_read_buffer(buffer_idx_);
  }
}

// ========== ShmManager Implementation ==========
ShmManager::ShmManager(const std::string &shm_name)
    : shm_name_(shm_name), shm_fd_(-1), shm_ptr_(nullptr),
      state_(ShmState::Uninitialized), is_creator_(false) {}

ShmManager::~ShmManager() { unmap_and_close(); }

void ShmManager::log_error(const std::string &message,
                           ShmStatus status_code) const {
  std::cerr << "Error [ShmManager '" << shm_name_ << "']: " << message
            << " (Status: " << shm_status_to_string(status_code) << " ["
            << static_cast<int>(status_code) << "])" << ". Errno: " << errno
            << " (" << strerror(errno) << ")" << std::endl;
}

void ShmManager::close_internal_handles() {
  if (shm_fd_ != -1) {
    if (close(shm_fd_) == -1) {
      std::cerr << "Warning [ShmManager '" << shm_name_
                << "]: Failed to close shm_fd. Errno: " << errno << std::endl;
    }
    shm_fd_ = -1;
  }
}

ShmStatus ShmManager::validate_buffer_layout(size_t shm_total_size,
                                             size_t buffer_size) const {
  size_t required_size = sizeof(ShmBufferControl) + NUM_BUFFERS * buffer_size;
  if (shm_total_size < required_size) {
    log_error("Shared memory size too small for buffer layout",
              ShmStatus::BufferTooSmall);
    return ShmStatus::BufferTooSmall;
  }
  return ShmStatus::Success;
}

ShmStatus ShmManager::create_and_init(size_t shm_total_size,
                                      size_t buffer_size) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Uninitialized) {
    log_error("Shared memory already initialized",
              ShmStatus::AlreadyInitialized);
    return ShmStatus::AlreadyInitialized;
  }
  ShmStatus validation_result =
      validate_buffer_layout(shm_total_size, buffer_size);
  if (validation_result != ShmStatus::Success) {
    return validation_result;
  }
  shm_fd_ = shm_open(shm_name_.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
  bool shm_newly_created = false;
  if (shm_fd_ == -1) {
    if (errno == EEXIST) {
      shm_fd_ = shm_open(shm_name_.c_str(), O_RDWR, 0666);
      if (shm_fd_ == -1) {
        log_error("Failed to open existing shared memory",
                  ShmStatus::ShmOpenFailed);
        return ShmStatus::ShmOpenFailed;
      }
      std::cout << "ShmManager '" << shm_name_
                << "': Opened existing shared memory." << std::endl;
    } else {
      log_error("Failed to create shared memory", ShmStatus::ShmOpenFailed);
      return ShmStatus::ShmOpenFailed;
    }
  } else {
    shm_newly_created = true;
    is_creator_ = true;
    if (ftruncate(shm_fd_, shm_total_size) == -1) {
      log_error("Failed to set shared memory size",
                ShmStatus::ShmTruncateFailed);
      close(shm_fd_);
      shm_fd_ = -1;
      shm_unlink(shm_name_.c_str());
      return ShmStatus::ShmTruncateFailed;
    }
    std::cout << "ShmManager '" << shm_name_
              << "': Created new shared memory with size " << shm_total_size
              << " bytes." << std::endl;
  }
  shm_ptr_ = mmap(nullptr, shm_total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                  shm_fd_, 0);
  if (shm_ptr_ == MAP_FAILED) {
    log_error("Failed to map shared memory", ShmStatus::ShmMapFailed);
    close(shm_fd_);
    shm_fd_ = -1;
    if (shm_newly_created) {
      shm_unlink(shm_name_.c_str());
    }
    shm_ptr_ = nullptr;
    return ShmStatus::ShmMapFailed;
  }
  current_shm_size_.store(shm_total_size, std::memory_order_release);
  buffer_size_.store(buffer_size, std::memory_order_release);
  if (shm_newly_created) {
    auto *control = static_cast<ShmBufferControl *>(shm_ptr_);
    control->initialize();
    std::cout << "ShmManager '" << shm_name_
              << "': Initialized buffer control structure." << std::endl;
  }
  state_ = ShmState::Created;
  std::cout << "ShmManager '" << shm_name_
            << "' created and mapped successfully." << std::endl;
  return ShmStatus::Success;
}

ShmStatus ShmManager::open_and_map(size_t shm_total_size, size_t buffer_size) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Uninitialized) {
    log_error("Shared memory already initialized",
              ShmStatus::AlreadyInitialized);
    return ShmStatus::AlreadyInitialized;
  }
  ShmStatus validation_result =
      validate_buffer_layout(shm_total_size, buffer_size);
  if (validation_result != ShmStatus::Success) {
    return validation_result;
  }
  shm_fd_ = shm_open(shm_name_.c_str(), O_RDWR, 0666);
  if (shm_fd_ == -1) {
    log_error("Failed to open shared memory", ShmStatus::ShmOpenFailed);
    return ShmStatus::ShmOpenFailed;
  }
  shm_ptr_ = mmap(nullptr, shm_total_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                  shm_fd_, 0);
  if (shm_ptr_ == MAP_FAILED) {
    log_error("Failed to map shared memory", ShmStatus::ShmMapFailed);
    close(shm_fd_);
    shm_fd_ = -1;
    shm_ptr_ = nullptr;
    return ShmStatus::ShmMapFailed;
  }
  current_shm_size_.store(shm_total_size, std::memory_order_release);
  buffer_size_.store(buffer_size, std::memory_order_release);
  is_creator_ = false;
  state_ = ShmState::Mapped;
  std::cout << "ShmManager '" << shm_name_
            << "' opened and mapped successfully." << std::endl;
  return ShmStatus::Success;
}

ShmStatus ShmManager::unmap_and_close() {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ == ShmState::Uninitialized || state_ == ShmState::Closed) {
    return ShmStatus::Success;
  }
  ShmStatus status = ShmStatus::Success;
  if (shm_ptr_ != nullptr) {
    size_t shm_size = current_shm_size_.load(std::memory_order_acquire);
    if (munmap(shm_ptr_, shm_size) == -1) {
      log_error("Failed to unmap shared memory", ShmStatus::ShmUnmapFailed);
      status = ShmStatus::ShmUnmapFailed;
    } else {
      std::cout << "ShmManager '" << shm_name_ << "': Unmapped successfully."
                << std::endl;
    }
    shm_ptr_ = nullptr;
    current_shm_size_.store(0, std::memory_order_release);
    buffer_size_.store(0, std::memory_order_release);
  }
  close_internal_handles();
  state_ = ShmState::Closed;
  return status;
}

ShmStatus ShmManager::unlink_shm() {
  if (shm_unlink(shm_name_.c_str()) == -1) {
    log_error("Failed to unlink shared memory", ShmStatus::ShmUnlinkFailed);
    return ShmStatus::ShmUnlinkFailed;
  } else {
    std::cout << "ShmManager '" << shm_name_
              << "': Unlinked (destroyed) successfully." << std::endl;
    return ShmStatus::Success;
  }
}

WriteBufferGuard ShmManager::acquire_write_buffer(size_t expected_size) {
  return WriteBufferGuard(this, expected_size);
}

ReadBufferGuard ShmManager::acquire_read_buffer() {
  return ReadBufferGuard(this);
}

// ========== 兼容性接口实现（内部使用零拷贝实现）==========
ShmStatus ShmManager::write_and_switch(const void *data, size_t size,
                                       uint64_t frame_version) {
  if (!data || size == 0) {
    return ShmStatus::InvalidArguments;
  }
  WriteBufferGuard guard = acquire_write_buffer(size);
  if (!guard.is_valid()) {
    // 写者没有获取到缓冲区，忙等或重试
    int retry_count = 1000;
    while (!guard.is_valid() && retry_count-- > 0) {
      std::this_thread::yield();
      guard = acquire_write_buffer(size);
    }
    if (!guard.is_valid()) {
      return ShmStatus::AcquireFailed;
    }
  }
  std::memcpy(guard.get(), data, size);
  return guard.commit(size, frame_version);
}

ShmStatus ShmManager::try_read_latest(void *data, size_t max_size,
                                      size_t *actual_size) {
  if (!data || max_size == 0) {
    return ShmStatus::InvalidArguments;
  }
  ReadBufferGuard guard = acquire_read_buffer();
  if (!guard.is_valid()) {
    if (actual_size)
      *actual_size = 0;
    return guard.status();
  }
  size_t copy_size = std::min(max_size, guard.size());
  std::memcpy(data, guard.get(), copy_size);
  if (actual_size)
    *actual_size = copy_size;
  return ShmStatus::Success;
}

ShmStatus ShmManager::wait_and_read(void *data, size_t max_size,
                                    size_t *actual_size) {
  while (true) {
    ReadBufferGuard guard = acquire_read_buffer();
    if (guard.is_valid()) {
      size_t copy_size = std::min(max_size, guard.size());
      std::memcpy(data, guard.get(), copy_size);
      if (actual_size)
        *actual_size = copy_size;
      return ShmStatus::Success;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return ShmStatus::NoDataAvailable; // 理论上不会执行到这里
}

// ========== 信息获取接口实现 ==========
void *ShmManager::get_shm_ptr() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return (state_ == ShmState::Created || state_ == ShmState::Mapped) ? shm_ptr_
                                                                     : nullptr;
}

ShmBufferControl *ShmManager::get_buffer_control() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return (state_ == ShmState::Created || state_ == ShmState::Mapped)
             ? static_cast<ShmBufferControl *>(shm_ptr_)
             : nullptr;
}

void *ShmManager::get_data_buffer(uint32_t buffer_idx) const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Created && state_ != ShmState::Mapped)
    return nullptr;
  if (buffer_idx >= NUM_BUFFERS)
    return nullptr;
  char *base = static_cast<char *>(shm_ptr_);
  return base + sizeof(ShmBufferControl) +
         buffer_idx * buffer_size_.load(std::memory_order_acquire);
}

size_t ShmManager::get_shm_size() const {
  return current_shm_size_.load(std::memory_order_acquire);
}

size_t ShmManager::get_buffer_size() const {
  return buffer_size_.load(std::memory_order_acquire);
}

ShmState ShmManager::get_state() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return state_;
}

bool ShmManager::is_initialized() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return state_ == ShmState::Created || state_ == ShmState::Mapped;
}

uint64_t ShmManager::get_frame_version(uint32_t buffer_idx) const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Created && state_ != ShmState::Mapped)
    return 0;
  auto *control = static_cast<ShmBufferControl *>(shm_ptr_);
  if (!control || buffer_idx >= NUM_BUFFERS)
    return 0;
  return control->frame_version[buffer_idx].load(std::memory_order_acquire);
}

// ========== 内部零拷贝实现方法 ==========
void *ShmManager::internal_acquire_write_buffer(size_t expected_size,
                                                uint32_t *buffer_idx) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Created && state_ != ShmState::Mapped)
    return nullptr;
  if (expected_size > buffer_size_.load(std::memory_order_acquire))
    return nullptr;
  auto *control = static_cast<ShmBufferControl *>(shm_ptr_);
  if (!control)
    return nullptr;
  uint32_t write_idx = 0;
  uint64_t min_version = UINT64_MAX;
  for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
    uint64_t current_version =
        control->frame_version[i].load(std::memory_order_acquire);
    if (current_version < min_version) {
      min_version = current_version;
      write_idx = i;
    }
  }
  if (control->buffer_reader_count[write_idx].load(std::memory_order_acquire) >
      0) {
    return nullptr;
  }
  control->buffer_ready[write_idx].store(false, std::memory_order_release);
  *buffer_idx = write_idx;
  return get_data_buffer(write_idx);
}

void ShmManager::internal_release_write_buffer(uint32_t buffer_idx) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Created && state_ != ShmState::Mapped)
    return;
  auto *control = static_cast<ShmBufferControl *>(shm_ptr_);
  if (control && buffer_idx < NUM_BUFFERS) {
    // 释放未提交的写缓冲区，不改变其状态
    // `buffer_ready` 仍然为 `false`，这样读者就不会读到它
  }
}

ShmStatus ShmManager::internal_commit_write_buffer(uint32_t buffer_idx,
                                                   size_t actual_size,
                                                   uint64_t frame_version) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Created && state_ != ShmState::Mapped)
    return ShmStatus::NotInitialized;
  auto *control = static_cast<ShmBufferControl *>(shm_ptr_);
  if (!control || buffer_idx >= NUM_BUFFERS)
    return ShmStatus::InvalidArguments;
  control->buffer_data_size[buffer_idx].store(actual_size,
                                              std::memory_order_release);
  control->frame_version[buffer_idx].store(frame_version,
                                           std::memory_order_release);
  control->buffer_ready[buffer_idx].store(true, std::memory_order_release);
  return ShmStatus::Success;
}

const void *ShmManager::internal_acquire_read_buffer(size_t *data_size,
                                                     uint64_t *frame_version,
                                                     uint32_t *buffer_idx,
                                                     ShmStatus *status) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  *status = ShmStatus::Success;
  auto *control = static_cast<ShmBufferControl *>(shm_ptr_);
  if (!control || (state_ != ShmState::Created && state_ != ShmState::Mapped)) {
    *status = ShmStatus::NotInitialized;
    return nullptr;
  }
  uint32_t latest_idx = 0;
  uint64_t max_version = 0;
  bool found_latest = false;
  for (uint32_t i = 0; i < NUM_BUFFERS; ++i) {
    // 确保缓冲区已就绪，并且版本号最新
    if (control->buffer_ready[i].load(std::memory_order_acquire)) {
      uint64_t current_version =
          control->frame_version[i].load(std::memory_order_acquire);
      if (current_version > max_version) {
        max_version = current_version;
        latest_idx = i;
        found_latest = true;
      }
    }
  }
  if (!found_latest) {
    *status = ShmStatus::NoDataAvailable;
    return nullptr;
  }
  // 增加读者计数
  control->buffer_reader_count[latest_idx].fetch_add(1,
                                                     std::memory_order_acquire);
  *buffer_idx = latest_idx;
  *data_size =
      control->buffer_data_size[latest_idx].load(std::memory_order_acquire);
  *frame_version =
      control->frame_version[latest_idx].load(std::memory_order_acquire);
  return get_data_buffer(latest_idx);
}

void ShmManager::internal_release_read_buffer(uint32_t buffer_idx) {
  std::lock_guard<std::mutex> lock(state_mutex_);
  if (state_ != ShmState::Created && state_ != ShmState::Mapped)
    return;
  auto *control = static_cast<ShmBufferControl *>(shm_ptr_);
  if (control && buffer_idx < NUM_BUFFERS) {
    control->buffer_reader_count[buffer_idx].fetch_sub(
        1, std::memory_order_release);
  }
}

// ========== C接口实现 ==========
extern "C" {
void *create_shm_manager(const char *shm_name) {
  if (!shm_name) {
    std::cerr << "Error: create_shm_manager received nullptr for name."
              << std::endl;
    return nullptr;
  }
  try {
    return new ShmManager(shm_name);
  } catch (const std::bad_alloc &e) {
    std::cerr << "Error: Failed to allocate ShmManager: " << e.what()
              << std::endl;
    return nullptr;
  }
}

void destroy_shm_manager(void *manager_ptr) {
  if (manager_ptr) {
    delete static_cast<ShmManager *>(manager_ptr);
  }
}

int shm_manager_create_and_init(void *manager_ptr, size_t shm_total_size,
                                size_t buffer_size) {
  if (!manager_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  return static_cast<int>(static_cast<ShmManager *>(manager_ptr)
                              ->create_and_init(shm_total_size, buffer_size));
}

int shm_manager_open_and_map(void *manager_ptr, size_t shm_total_size,
                             size_t buffer_size) {
  if (!manager_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  return static_cast<int>(static_cast<ShmManager *>(manager_ptr)
                              ->open_and_map(shm_total_size, buffer_size));
}

int shm_manager_unmap_and_close(void *manager_ptr) {
  if (!manager_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  return static_cast<int>(
      static_cast<ShmManager *>(manager_ptr)->unmap_and_close());
}

int shm_manager_unlink_shm(void *manager_ptr) {
  if (!manager_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  return static_cast<int>(static_cast<ShmManager *>(manager_ptr)->unlink_shm());
}

// 零拷贝C接口
void *shm_manager_acquire_write_buffer(void *manager_ptr,
                                       size_t expected_size) {
  if (!manager_ptr)
    return nullptr;
  ShmManager *manager = static_cast<ShmManager *>(manager_ptr);
  auto guard = std::make_unique<WriteBufferGuard>(
      manager->acquire_write_buffer(expected_size));
  if (guard->is_valid()) {
    void *buffer_ptr = guard->get();
    std::lock_guard<std::mutex> lock(guard_map_mutex);
    write_guards[buffer_ptr] = std::move(guard);
    return buffer_ptr;
  }
  return nullptr;
}

int shm_manager_commit_write_buffer(void *manager_ptr, void *buffer_ptr,
                                    size_t actual_size,
                                    uint64_t frame_version) {
  if (!manager_ptr || !buffer_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  std::lock_guard<std::mutex> lock(guard_map_mutex);
  auto it = write_guards.find(buffer_ptr);
  if (it == write_guards.end())
    return static_cast<int>(ShmStatus::InvalidArguments);
  ShmStatus status = it->second->commit(actual_size, frame_version);
  write_guards.erase(it); // Guard自动析构，完成提交
  return static_cast<int>(status);
}

void shm_manager_release_write_buffer(void *manager_ptr, void *buffer_ptr) {
  if (!manager_ptr || !buffer_ptr)
    return;
  std::lock_guard<std::mutex> lock(guard_map_mutex);
  auto it = write_guards.find(buffer_ptr);
  if (it != write_guards.end()) {
    write_guards.erase(it); // Guard自动析构，释放缓冲区
  }
}

const void *shm_manager_acquire_read_buffer(void *manager_ptr,
                                            size_t *data_size,
                                            uint64_t *frame_version) {
  if (!manager_ptr || !data_size || !frame_version)
    return nullptr;
  ShmManager *manager = static_cast<ShmManager *>(manager_ptr);
  auto guard =
      std::make_unique<ReadBufferGuard>(manager->acquire_read_buffer());
  if (guard->is_valid()) {
    const void *buffer_ptr = guard->get();
    *data_size = guard->size();
    *frame_version = guard->frame_version();
    std::lock_guard<std::mutex> lock(guard_map_mutex);
    read_guards[buffer_ptr] = std::move(guard);
    return buffer_ptr;
  }
  return nullptr;
}

const void *shm_manager_wait_for_data(void *manager_ptr, size_t *data_size,
                                      uint64_t *frame_version) {
  if (!manager_ptr || !data_size || !frame_version)
    return nullptr;
  ShmManager *manager = static_cast<ShmManager *>(manager_ptr);
  while (true) {
    auto guard =
        std::make_unique<ReadBufferGuard>(manager->acquire_read_buffer());
    if (guard->is_valid()) {
      const void *buffer_ptr = guard->get();
      *data_size = guard->size();
      *frame_version = guard->frame_version();
      std::lock_guard<std::mutex> lock(guard_map_mutex);
      read_guards[buffer_ptr] = std::move(guard);
      return buffer_ptr;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return nullptr;
}

void shm_manager_release_read_buffer(void *manager_ptr,
                                     const void *buffer_ptr) {
  if (!manager_ptr || !buffer_ptr)
    return;
  std::lock_guard<std::mutex> lock(guard_map_mutex);
  auto it = read_guards.find(buffer_ptr);
  if (it != read_guards.end()) {
    read_guards.erase(it); // Guard自动析构，释放缓冲区
  }
}

// 兼容接口
int shm_manager_write_and_switch(void *manager_ptr, const void *data,
                                 size_t size, uint64_t frame_version) {
  if (!manager_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  return static_cast<int>(static_cast<ShmManager *>(manager_ptr)
                              ->write_and_switch(data, size, frame_version));
}

int shm_manager_try_read_latest(void *manager_ptr, void *data, size_t max_size,
                                size_t *actual_size) {
  if (!manager_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  return static_cast<int>(static_cast<ShmManager *>(manager_ptr)
                              ->try_read_latest(data, max_size, actual_size));
}

int shm_manager_wait_and_read(void *manager_ptr, void *data, size_t max_size,
                              size_t *actual_size) {
  if (!manager_ptr)
    return static_cast<int>(ShmStatus::InvalidArguments);
  return static_cast<int>(static_cast<ShmManager *>(manager_ptr)
                              ->wait_and_read(data, max_size, actual_size));
}

// 信息获取接口
void *shm_manager_get_shm_ptr(const void *manager_ptr) {
  if (!manager_ptr)
    return nullptr;
  return static_cast<const ShmManager *>(manager_ptr)->get_shm_ptr();
}

size_t shm_manager_get_shm_size(const void *manager_ptr) {
  if (!manager_ptr)
    return 0;
  return static_cast<const ShmManager *>(manager_ptr)->get_shm_size();
}

size_t shm_manager_get_buffer_size(const void *manager_ptr) {
  if (!manager_ptr)
    return 0;
  return static_cast<const ShmManager *>(manager_ptr)->get_buffer_size();
}

uint64_t shm_manager_get_frame_version(const void *manager_ptr,
                                       uint32_t buffer_idx) {
  if (!manager_ptr)
    return 0;
  return static_cast<const ShmManager *>(manager_ptr)
      ->get_frame_version(buffer_idx);
}
}
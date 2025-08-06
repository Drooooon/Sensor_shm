#include "video_capture.h"
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

VideoCaptureModule::VideoCaptureModule() {}

VideoCaptureModule::~VideoCaptureModule() {
  stop();
  cleanupSharedMemory();
}

bool VideoCaptureModule::initialize(int camera_index) {
  camera_index_ = camera_index;
  if (!setupSharedMemory()) {
    std::cerr << "[VideoCapture] Shared memory setup failed.\n";
    return false;
  }
  return true;
}

void VideoCaptureModule::start() {
  running_ = true;
  capture_thread_ = std::thread(&VideoCaptureModule::captureLoop, this);
}

void VideoCaptureModule::stop() {
  running_ = false;
  if (capture_thread_.joinable())
    capture_thread_.join();
}

void VideoCaptureModule::captureLoop() {
  int current_camera_index = camera_index_;

  cap_.open(camera_index_);
  if (!cap_.isOpened()) {
    std::cerr << "[VideoCapture] Failed to open camera.\n";
    return;
    }

    std::cout << "[VideoCapture] Capture started.\n";

    while (running_) {
      if (ctrl_->paused) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      if (ctrl_->camera_index != current_camera_index) {
        current_camera_index = ctrl_->camera_index;
        cap_.release();
        cap_.open(current_camera_index);
        if (!cap_.isOpened()) {
          std::cerr << "[VideoCapture] Failed to open camera "
                    << current_camera_index << ".\n";
          std::this_thread::sleep_for(std::chrono::seconds(1));
          continue;
        }
        std::cout << "[VideoCapture] Switched to camera "
                  << current_camera_index << ".\n";

        // 切换摄像头后重置宽高设置
        last_width_ = -1;
        last_height_ = -1;
      }

      if (ctrl_->width != last_width_ || ctrl_->height != last_height_) {
        cap_.set(cv::CAP_PROP_FRAME_WIDTH, ctrl_->width);
        cap_.set(cv::CAP_PROP_FRAME_HEIGHT, ctrl_->height);
        last_width_ = ctrl_->width;
        last_height_ = ctrl_->height;
      }

      cv::Mat frame;
      cap_ >> frame;
      if (frame.empty())
        continue;

      std::vector<uchar> jpeg_buf;
      cv::imencode(".jpg", frame, jpeg_buf);
      if (jpeg_buf.size() > SHM_SIZE - sizeof(CaptureControl)) {
        std::cerr << "[VideoCapture] JPEG too large.\n";
        continue;
      }

      memcpy(jpeg_data_, jpeg_buf.data(), jpeg_buf.size());
      ctrl_->jpeg_size = jpeg_buf.size();
      ctrl_->frame_id++;

      std::this_thread::sleep_for(
          std::chrono::milliseconds(ctrl_->interval_ms));
    }

    cap_.release();
    std::cout << "[VideoCapture] Capture stopped.\n";
  }

bool VideoCaptureModule::setupSharedMemory() {
  shm_fd_ = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd_ < 0) {
    std::cerr << "[VideoCapture] Failed to create shared memory.\n";
    return false;
  }

  // 修复：检查 ftruncate 返回值
  if (ftruncate(shm_fd_, SHM_SIZE) != 0) {
    std::cerr << "[VideoCapture] Failed to set shared memory size.\n";
    close(shm_fd_);
    shm_unlink(SHM_NAME);
    return false;
  }

  shm_ptr_ = (unsigned char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd_, 0);
  if (shm_ptr_ == MAP_FAILED) {
    std::cerr << "[VideoCapture] Failed to map shared memory.\n";
    close(shm_fd_);
    shm_unlink(SHM_NAME);
    return false;
  }

  ctrl_ = reinterpret_cast<CaptureControl *>(shm_ptr_);
  jpeg_data_ = shm_ptr_ + sizeof(CaptureControl);

  // 初始化控制块
  ctrl_->frame_id = 0;
  ctrl_->jpeg_size = 0;
  ctrl_->paused = false;
  ctrl_->width = 640;
  ctrl_->height = 480;
  ctrl_->interval_ms = 33; // 默认约30fps
  ctrl_->camera_index = camera_index_;

  return true;
}

void VideoCaptureModule::cleanupSharedMemory() {
  if (shm_ptr_ && shm_ptr_ != MAP_FAILED) {
    munmap(shm_ptr_, SHM_SIZE);
    shm_ptr_ = nullptr;
  }
  if (shm_fd_ != -1) {
    close(shm_fd_);
    shm_unlink(SHM_NAME);
    shm_fd_ = -1;
  }
}
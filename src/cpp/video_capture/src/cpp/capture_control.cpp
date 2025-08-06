#include "capture_control.h"
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>


CaptureControlWrapper::CaptureControlWrapper() {}

CaptureControlWrapper::~CaptureControlWrapper() {
  if (shm_ptr_)
    munmap(shm_ptr_, SHM_SIZE);
  if (shm_fd_ != -1)
    close(shm_fd_);
}

bool CaptureControlWrapper::initialize() {
  shm_fd_ = shm_open(SHM_NAME, O_RDWR, 0666);
  if (shm_fd_ < 0) {
    std::cerr << "[CaptureControl] shm_open failed\n";
    return false;
  }

  shm_ptr_ = (unsigned char *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, shm_fd_, 0);
  if (shm_ptr_ == MAP_FAILED) {
    std::cerr << "[CaptureControl] mmap failed\n";
    return false;
  }

  ctrl_ = reinterpret_cast<CaptureControl *>(shm_ptr_);
  return true;
}

void CaptureControlWrapper::setPaused(bool paused) {
  if (ctrl_)
    ctrl_->paused = paused;
}

bool CaptureControlWrapper::getPaused() {
  if (ctrl_)
    return ctrl_->paused.load();
  return false;
}

void CaptureControlWrapper::setResolution(uint32_t width, uint32_t height) {
  if (ctrl_) {
    ctrl_->width = width;
    ctrl_->height = height;
  }
}

void CaptureControlWrapper::getResolution(uint32_t &width, uint32_t &height) {
  if (ctrl_) {
    width = ctrl_->width.load();
    height = ctrl_->height.load();
  }
}

void CaptureControlWrapper::setFPS(uint32_t fps) {
  if (ctrl_) {
    if (fps == 0)
      fps = 30;
    ctrl_->interval_ms = 1000 / fps;
  }
}

uint32_t CaptureControlWrapper::getFPS() {
  if (ctrl_) {
    uint32_t interval = ctrl_->interval_ms.load();
    if (interval == 0)
      return 30;
    return 1000 / interval;
  }
  return 30;
}

uint32_t CaptureControlWrapper::getFrameID() {
  if (ctrl_)
    return ctrl_->frame_id.load();
  return 0;
}

uint32_t CaptureControlWrapper::getJPEGSize() {
  if (ctrl_)
    return ctrl_->jpeg_size.load();
  return 0;
}

unsigned char *CaptureControlWrapper::getJPEGData() {
  if (shm_ptr_) {
    return shm_ptr_ + sizeof(CaptureControl);
  }
  return nullptr;
}

void CaptureControlWrapper::setCameraIndex(int index) {
  if (ctrl_) {
    ctrl_->camera_index = index;
  }
}

int CaptureControlWrapper::getCameraIndex() {
  if (ctrl_) {
    return ctrl_->camera_index.load();
  }
  return 0;
}
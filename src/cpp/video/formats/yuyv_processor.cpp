// ==========================================================
//    src/cpp/video/formats/yuyv_processor.cpp (最终修复版)
// ==========================================================
#include "yuyv_processor.h"
#include "config/config_manager.h"

#include <cstring> // for strerror
#include <fcntl.h>
#include <iostream>
#include <linux/videodev2.h>
#include <poll.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

struct V4l2Capture::Buffer {
  void *start;
  size_t length;
};

// *** 关键修复：所有函数实现都使用 V4l2Capture:: ***
V4l2Capture::V4l2Capture(const V4l2Config &config)
    : config_(config), fd_(-1), buffers_(nullptr), buffer_count_(0),
      is_streaming_(false) {
  try {
    open_device();
    init_format();
    init_mmap();
  } catch (...) {
    if (fd_ != -1)
      close(fd_);
    throw;
  }
}

V4l2Capture::~V4l2Capture() {
  if (is_streaming_) {
    try {
      stop_stream();
    } catch (...) {
    } // Destructors should not throw
  }
  if (buffers_) {
    for (size_t i = 0; i < buffer_count_; ++i) {
      if (buffers_[i].start)
        munmap(buffers_[i].start, buffers_[i].length);
    }
    delete[] buffers_;
  }
  if (fd_ != -1)
    close(fd_);
  std::cout << "V4l2Capture cleaned up." << std::endl;
}

void V4l2Capture::open_device() {
  fd_ = open(config_.device_path.c_str(), O_RDWR);
  if (fd_ == -1)
    throw std::runtime_error("Failed to open device: " + config_.device_path);
}

void V4l2Capture::init_format() {
  v4l2_format fmt{};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = config_.width;
  fmt.fmt.pix.height = config_.height;
  fmt.fmt.pix.pixelformat = config_.pixel_format_v4l2;
  fmt.fmt.pix.field = V4L2_FIELD_NONE;
  xioctl(VIDIOC_S_FMT, &fmt);
}

void V4l2Capture::init_mmap() {
  v4l2_requestbuffers req{};
  req.count = config_.buffer_count;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  xioctl(VIDIOC_REQBUFS, &req);

  if (req.count < 2)
    throw std::runtime_error("Insufficient buffer memory.");
  buffer_count_ = req.count;
  buffers_ = new Buffer[buffer_count_];

  for (size_t i = 0; i < buffer_count_; ++i) {
    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    xioctl(VIDIOC_QUERYBUF, &buf);
    buffers_[i].length = buf.length;
    buffers_[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd_, buf.m.offset);
    if (buffers_[i].start == MAP_FAILED)
      throw std::runtime_error("mmap failed");
  }
}

void V4l2Capture::start_stream() {
  if (is_streaming_)
    return;
  for (size_t i = 0; i < buffer_count_; ++i) {
    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    xioctl(VIDIOC_QBUF, &buf);
  }
  v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(VIDIOC_STREAMON, &type);
  is_streaming_ = true;
}

void V4l2Capture::stop_stream() {
  if (!is_streaming_)
    return;
  v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(VIDIOC_STREAMOFF, &type);
  is_streaming_ = false;
}

bool V4l2Capture::capture_frame(V4l2FrameData &out_frame,
                                std::atomic<bool> &running) {
  pollfd pfd = {fd_, POLLIN, 0};
  int ret = poll(&pfd, 1, 200);
  if (ret < 0)
    return false;
  if (ret == 0)
    return true;

  if (!running.load())
    return false;

  v4l2_buffer buf{};
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  if (ioctl(fd_, VIDIOC_DQBUF, &buf) == -1)
    return false;

  out_frame.data = static_cast<const uint8_t *>(buffers_[buf.index].start);
  out_frame.size = buf.bytesused;
  out_frame.width = config_.width;
  out_frame.height = config_.height;

  xioctl(VIDIOC_QBUF, &buf);
  return true;
}

void V4l2Capture::xioctl(unsigned long request, void *arg) {
  if (ioctl(fd_, request, arg) == -1) {
    throw std::runtime_error("ioctl failed: " + std::string(strerror(errno)));
  }
}
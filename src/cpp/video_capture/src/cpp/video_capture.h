#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "capture_control.h" 
#include <atomic>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <thread>

class VideoCaptureModule {
public:
  VideoCaptureModule();
  ~VideoCaptureModule();

  // 初始化摄像头索引，建立共享内存等，成功返回true
  bool initialize(int camera_index);

  // 开始采集线程
  void start();

  // 停止采集线程
  void stop();

private:
  // 采集线程函数
  void captureLoop();

  // 共享内存初始化和清理
  bool setupSharedMemory();
  void cleanupSharedMemory();

private:
  int camera_index_;
  std::atomic<bool> running_;
  std::thread capture_thread_;
  cv::VideoCapture cap_;

  int last_width_ = -1;
  int last_height_ = -1;

  // 共享内存描述符及指针
  int shm_fd_ = -1;
  unsigned char *shm_ptr_ = nullptr;
  CaptureControl *ctrl_ = nullptr;
  unsigned char *jpeg_data_ = nullptr;

  // 共享内存配置
  const char *shm_name_ = SHM_NAME; // 使用宏值初始化
  const size_t shm_size_ = SHM_SIZE;
};

#endif // VIDEO_CAPTURE_H
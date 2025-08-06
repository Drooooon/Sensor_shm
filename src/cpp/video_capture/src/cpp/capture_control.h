#ifndef CAPTURE_CONTROL_H
#define CAPTURE_CONTROL_H

#include <atomic>
#include <cstdint>
#include <cstddef>

#define SHM_NAME "/video_frame_shm"
#define SHM_SIZE (10 * 1024 * 1024) // 10MB

struct CaptureControl {
  std::atomic<uint32_t> frame_id;
  std::atomic<uint32_t> jpeg_size;
  std::atomic<bool> paused;
  std::atomic<uint32_t> width;
  std::atomic<uint32_t> height;
  std::atomic<uint32_t> interval_ms;
  std::atomic<int> camera_index;
};

class CaptureControlWrapper {
public:
  CaptureControlWrapper();
  ~CaptureControlWrapper();

  bool initialize();
  void setPaused(bool paused);
  bool getPaused();
  void setResolution(uint32_t width, uint32_t height);
  void getResolution(uint32_t &width, uint32_t &height);
  void setFPS(uint32_t fps);
  uint32_t getFPS();
  uint32_t getFrameID();
  uint32_t getJPEGSize();

  // 新增：获取JPEG数据
  unsigned char *getJPEGData();

  void setCameraIndex(int index);
  int getCameraIndex();

private:

  int shm_fd_ = -1;
  unsigned char *shm_ptr_ = nullptr;
  CaptureControl *ctrl_ = nullptr;
};

#endif
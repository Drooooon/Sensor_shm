#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "common/json/nlohmann_json/include/nlohmann/json.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>

// 视频捕获配置 (V4L2)
struct V4l2Config {
  std::string device_path;
  int width;
  int height;
  uint32_t pixel_format_v4l2; // 存储转换后的 V4L2_PIX_FMT_*
  int buffer_count;
};

// 共享内存传输配置
struct ShmConfig {
  std::string name;
  size_t total_size_bytes;
  size_t buffer_size_bytes;
};

class ConfigManager {
public:
  static ConfigManager &get_instance();

  // 分别加载不同的配置文件
  void load_video_config(const std::string &path);
  void load_shm_config(const std::string &path);

  const V4l2Config &get_v4l2_config() const;
  const ShmConfig &get_shm_config() const;

  ConfigManager(const ConfigManager &) = delete;
  ConfigManager &operator=(const ConfigManager &) = delete;

private:
  ConfigManager() : video_loaded_(false), shm_loaded_(false) {}

  bool video_loaded_;
  bool shm_loaded_;
  V4l2Config v4l2_config_;
  ShmConfig shm_config_;
};

#endif // CONFIG_MANAGER_H
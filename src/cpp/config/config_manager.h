/**
 * @file config_manager.h
 * @brief 配置管理器头文件，提供系统配置的统一管理接口
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件定义了配置管理器类，用于加载和管理视频捕获配置和共享内存配置。
 * 采用单例模式确保配置的全局一致性。
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "common/json/nlohmann_json/include/nlohmann/json.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>

/**
 * @brief 视频捕获配置结构体 (V4L2)
 *
 * 存储V4L2视频设备的配置参数，包括设备路径、分辨率、像素格式等
 */
struct V4l2Config {
  std::string device_path; ///< 视频设备路径，如 "/dev/video0"
  int width;               ///< 视频帧宽度（像素）
  int height;              ///< 视频帧高度（像素）
  uint32_t pixel_format_v4l2; ///< 像素格式，存储转换后的 V4L2_PIX_FMT_* 常量
  int buffer_count;           ///< 缓冲区数量
};

/**
 * @brief 共享内存传输配置结构体
 *
 * 定义共享内存的大小、缓冲区配置等参数，用于进程间高效数据传输
 */
struct ShmConfig {
  std::string name;         ///< 共享内存名称，用于标识和访问
  size_t total_size_bytes;  ///< 总共享内存大小（字节）
  size_t buffer_size_bytes; ///< 单个缓冲区大小（字节）
  uint32_t buffer_count;    ///< 缓冲区数量
};

/**
 * @brief 配置管理器类
 *
 * 采用单例模式的配置管理器，负责加载、验证和提供系统配置。
 * 支持分别加载视频配置和共享内存配置，确保配置的完整性和一致性。
 */
class ConfigManager {
public:
  /**
   * @brief 获取配置管理器单例实例
   * @return ConfigManager& 配置管理器实例的引用
   */
  static ConfigManager &get_instance();

  /**
   * @brief 加载视频配置文件
   * @param path 配置文件路径
   * @throws std::runtime_error 当文件无法打开或格式错误时抛出异常
   */
  void load_video_config(const std::string &path);

  /**
   * @brief 加载共享内存配置文件
   * @param path 配置文件路径
   * @throws std::runtime_error 当文件无法打开或格式错误时抛出异常
   */
  void load_shm_config(const std::string &path);

  /**
   * @brief 获取V4L2视频配置
   * @return const V4l2Config& V4L2配置的常量引用
   * @throws std::runtime_error 当视频配置未加载时抛出异常
   */
  const V4l2Config &get_v4l2_config() const;

  /**
   * @brief 获取共享内存配置
   * @return const ShmConfig& 共享内存配置的常量引用
   * @throws std::runtime_error 当共享内存配置未加载时抛出异常
   */
  const ShmConfig &get_shm_config() const;

  // 删除拷贝构造和赋值操作符，确保单例模式
  ConfigManager(const ConfigManager &) = delete;
  ConfigManager &operator=(const ConfigManager &) = delete;

private:
  /**
   * @brief 私有构造函数，初始化配置加载状态
   */
  ConfigManager() : video_loaded_(false), shm_loaded_(false) {}

  bool video_loaded_;      ///< 视频配置是否已加载的标志
  bool shm_loaded_;        ///< 共享内存配置是否已加载的标志
  V4l2Config v4l2_config_; ///< V4L2视频配置实例
  ShmConfig shm_config_;   ///< 共享内存配置实例
};

#endif // CONFIG_MANAGER_H
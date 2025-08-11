/**
 * @file config_manager.cpp
 * @brief 配置管理器实现文件
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 实现配置管理器的各项功能，包括JSON配置文件的解析、V4L2格式转换等
 */

#include "config_manager.h"
#include <fstream>
#include <iostream>
#include <linux/videodev2.h> // 需要 V4L2 格式常量
#include <map>

ConfigManager &ConfigManager::get_instance() {
  static ConfigManager instance;
  return instance;
}

/**
 * @brief 将字符串格式转换为 V4L2 像素格式常量
 * @param format_str 格式字符串，如 "YUYV" 或 "MJPG"
 * @return uint32_t 对应的 V4L2_PIX_FMT_* 常量值
 * @throws std::runtime_error 当格式字符串不被支持时抛出异常
 *
 * 该函数维护一个静态映射表，将常见的格式字符串转换为V4L2系统识别的格式常量。
 * 目前支持的格式包括：
 * - "YUYV" -> V4L2_PIX_FMT_YUYV
 * - "MJPG" -> V4L2_PIX_FMT_MJPEG
 */
static uint32_t string_to_v4l2_format(const std::string &format_str) {
  static const std::map<std::string, uint32_t> format_map = {
      {"YUYV", V4L2_PIX_FMT_YUYV}, {"MJPG", V4L2_PIX_FMT_MJPEG}};
  auto it = format_map.find(format_str);
  if (it != format_map.end()) {
    return it->second;
  }
  throw std::runtime_error("Config Error: Unknown format string '" +
                           format_str + "'");
}

void ConfigManager::load_video_config(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("Config Error: Could not open video config at '" +
                             path + "'");

  nlohmann::json data;
  file >> data;

  const auto &cfg = data.at("v4l2_capture");
  v4l2_config_.device_path = cfg.at("device_path");
  v4l2_config_.width = cfg.at("width");
  v4l2_config_.height = cfg.at("height");
  v4l2_config_.pixel_format_v4l2 = string_to_v4l2_format(cfg.at("format"));
  v4l2_config_.buffer_count = cfg.at("buffer_count");

  video_loaded_ = true;
  std::cout << "Video config loaded from " << path << std::endl;
}

void ConfigManager::load_shm_config(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("Config Error: Could not open shm config at '" +
                             path + "'");

  nlohmann::json data;
  file >> data;

  const auto &cfg = data.at("shared_memory");
  shm_config_.name = cfg.at("name");
  shm_config_.total_size_bytes = (size_t)cfg.at("total_size_mb") * 1024 * 1024;
  shm_config_.buffer_size_bytes =
      (size_t)cfg.at("buffer_size_mb") * 1024 * 1024;
  shm_config_.buffer_count = cfg.at("buffer_count");

  shm_loaded_ = true;
  std::cout << "SHM config loaded from " << path << std::endl;
}

const V4l2Config &ConfigManager::get_v4l2_config() const {
  if (!video_loaded_)
    throw std::runtime_error("Video config not loaded.");
  return v4l2_config_;
}

const ShmConfig &ConfigManager::get_shm_config() const {
  if (!shm_loaded_)
    throw std::runtime_error("SHM config not loaded.");
  return shm_config_;
}
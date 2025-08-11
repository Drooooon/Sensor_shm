/**
 * @file image_shm_manager.h
 * @brief 图像共享内存管理器
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件定义了专门用于图像数据传输的共享内存管理器，
 * 继承自通用共享内存管理器，添加了图像特定的功能。
 */

#ifndef IMAGE_SHM_MANAGER_H
#define IMAGE_SHM_MANAGER_H

#include "common/ipc/shm_manager.h"
#include <cstdint>
#include <string>

/**
 * @brief 支持的图像格式枚举
 *
 * 定义了系统支持的各种图像格式类型。
 */
enum class ImageFormat {
  YUYV, ///< YUYV 422格式，常用于USB摄像头
  H264, ///< H.264视频编码格式
  BGR,  ///< OpenCV标准的BGR格式
  MJPG  ///< Motion JPEG压缩格式
};

/**
 * @brief 图像头部信息结构
 *
 * 存储图像的元数据信息，包括格式、尺寸、通道数等。
 * 该结构体会与图像数据一起存储在共享内存中。
 */
struct ImageHeader {
  ImageFormat format; ///< 图像格式
  uint32_t width;     ///< 图像宽度（像素）
  uint32_t height;    ///< 图像高度（像素）
  uint32_t channels;  ///< 颜色通道数（如RGB为3，RGBA为4）
  uint32_t data_size; ///< 图像数据大小（字节）
  uint8_t frame_type; ///< 帧类型标志（如关键帧、差分帧等）
};

/**
 * @brief 图像共享内存管理器类
 *
 * 继承自通用共享内存管理器，专门用于处理图像数据的传输。
 * 在原有功能基础上增加了图像特定的写入和读取方法，
 * 自动处理图像头部信息和数据的组织。
 *
 * 主要特性：
 * - 自动管理图像头部和数据的存储布局
 * - 支持多种图像格式的传输
 * - 保持帧版本和时间戳信息
 * - 提供类型安全的图像数据访问接口
 */
class ImageShmManager : public ShmManager {
public:
  /**
   * @brief 构造函数
   * @param shm_name 共享内存名称
   *
   * 初始化图像共享内存管理器，继承父类的所有功能。
   */
  ImageShmManager(const std::string &shm_name) : ShmManager(shm_name) {}

  /**
   * @brief 写入图像数据到共享内存
   * @param image_data 图像数据指针
   * @param image_data_size 图像数据大小（字节）
   * @param width 图像宽度（像素）
   * @param height 图像高度（像素）
   * @param channels 颜色通道数
   * @param frame_version 帧版本号，用于标识数据更新
   * @param format 图像格式
   * @param frame_type 帧类型标志，默认为0
   * @return ShmStatus 操作结果状态码
   *
   * 将图像数据和相关元数据写入共享内存。方法会自动创建
   * ImageHeader结构体并与图像数据一起存储。
   *
   * @note 实际存储格式为：[ImageHeader][图像数据]
   */
  ShmStatus write_image(const uint8_t *image_data, size_t image_data_size,
                        uint32_t width, uint32_t height, uint32_t channels,
                        uint64_t frame_version, ImageFormat format,
                        uint8_t frame_type = 0);

  /**
   * @brief 从共享内存读取图像数据
   * @param out_buffer 输出缓冲区，接收图像数据
   * @param max_buffer_size 输出缓冲区的最大容量
   * @param out_width 输出参数，接收图像宽度
   * @param out_height 输出参数，接收图像高度
   * @param out_channels 输出参数，接收通道数
   * @param out_data_size 输出参数，接收实际数据大小
   * @param out_frame_version 输出参数，接收帧版本号
   * @param out_timestamp_us 输出参数，接收时间戳（微秒）
   * @param out_format 输出参数，接收图像格式
   * @param out_frame_type 输出参数，接收帧类型
   * @return ShmStatus 操作结果状态码
   *
   * 从共享内存读取最新的图像数据和元数据。方法会自动解析
   * ImageHeader并提取图像数据到输出缓冲区。
   *
   * @warning out_buffer必须足够大以容纳图像数据
   */
  ShmStatus read_image(uint8_t *out_buffer, size_t max_buffer_size,
                       uint32_t *out_width, uint32_t *out_height,
                       uint32_t *out_channels, size_t *out_data_size,
                       uint64_t *out_frame_version, uint64_t *out_timestamp_us,
                       ImageFormat *out_format, uint8_t *out_frame_type);

private:
  static constexpr size_t HEADER_SIZE =
      sizeof(ImageHeader); ///< 图像头部大小常量
};

#endif // IMAGE_SHM_MANAGER_H
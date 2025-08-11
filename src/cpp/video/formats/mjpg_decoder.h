/**
 * @file mjpg_decoder.h
 * @brief MJPEG格式图像解码器
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件实现了MJPEG（Motion JPEG）格式到BGR格式的解码器。
 * MJPEG是基于JPEG压缩的视频格式，每帧都是独立的JPEG图像。
 */

#ifndef MJPG_DECODER_H
#define MJPG_DECODER_H

#include "decoder_interface.h"

/**
 * @brief MJPEG格式解码器类
 *
 * 实现IDecoder接口，专门用于将MJPEG（Motion JPEG）格式的压缩图像数据
 * 解码为OpenCV标准的BGR格式。
 *
 * MJPEG格式特点：
 * - 每帧都是独立的JPEG图像
 * - 压缩效率高，适合网络传输
 * - 解码速度快，支持硬件加速
 * - 广泛用于网络摄像头和视频会议
 *
 * 解码过程利用OpenCV内置的JPEG解码器，确保高效和稳定的性能。
 */
class MjpgDecoder : public IDecoder {
public:
  /**
   * @brief 解码MJPEG格式图像数据
   * @param data MJPEG格式的压缩图像数据指针
   * @param header 图像头部信息，包含数据大小等元数据
   * @return cv::Mat 解码后的BGR格式OpenCV矩阵
   * @throws std::runtime_error 当数据无效或解码失败时抛出异常
   *
   * 将MJPEG压缩数据解码为BGR格式：
   * 1. 验证输入数据的完整性和有效性
   * 2. 使用OpenCV的imdecode函数解码JPEG数据
   * 3. 检查解码结果的有效性
   * 4. 确保输出格式为BGR（OpenCV默认格式）
   * 5. 返回解码后的Mat对象
   *
   * @note 输入数据必须是有效的JPEG格式
   * @warning data指针必须指向完整的JPEG数据块
   */
  cv::Mat decode(const uint8_t *data, const ImageHeader &header) override;
};

#endif // MJPG_DECODER_H
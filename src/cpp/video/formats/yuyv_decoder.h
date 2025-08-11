/**
 * @file yuyv_decoder.h
 * @brief YUYV格式图像解码器
 * @author SensorComm Team
 * @date 2025-08-11
 * @version 1.0
 *
 * 该文件实现了YUYV（YUV422）格式到BGR格式的解码器。
 * YUYV是一种常见的YUV颜色空间格式，广泛用于USB摄像头。
 */

#ifndef YUYV_DECODER_H
#define YUYV_DECODER_H

#include "decoder_interface.h"

/**
 * @brief YUYV格式解码器类
 *
 * 实现IDecoder接口，专门用于将YUYV（YUV422）格式的图像数据
 * 转换为OpenCV标准的BGR格式。
 *
 * YUYV格式特点：
 * - 每个像素占用2字节
 * - Y分量（亮度）对每个像素都有
 * - U和V分量（色度）每两个像素共享
 * - 存储顺序：Y0 U0 Y1 V0（对应两个像素）
 *
 * 转换算法基于ITU-R BT.601标准的YUV到RGB转换公式。
 */
class YuyvDecoder : public IDecoder {
public:
  /**
   * @brief 解码YUYV格式图像数据
   * @param data YUYV格式的原始图像数据指针
   * @param header 图像头部信息，包含宽度、高度等元数据
   * @return cv::Mat 解码后的BGR格式OpenCV矩阵
   * @throws std::runtime_error 当数据无效或解码失败时抛出异常
   *
   * 将YUYV格式的原始数据转换为BGR格式：
   * 1. 验证数据完整性（大小应为width * height * 2）
   * 2. 逐像素提取Y、U、V分量
   * 3. 应用YUV到RGB的转换公式
   * 4. 转换颜色通道顺序（RGB到BGR）
   * 5. 返回OpenCV Mat对象
   *
   * @note 输入数据大小必须等于 width * height * 2 字节
   * @warning data指针必须指向有效的YUYV数据
   */
  cv::Mat decode(const uint8_t *data, const ImageHeader &header) override;
};

#endif // YUYV_DECODER_H
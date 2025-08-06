#include "image_shm_manager.h"
#include <iostream>
// #include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libswscale/swscale.h>
#include <opencv2/opencv.hpp>
#include <stdexcept>

int main() {
  // 新版FFmpeg不再需要avformat_network_init()
  // avformat_network_init(); // 移除此行

  // Initialize shared memory - only need YUYV
  ImageShmManager yuyv_shm("yuyv_shm");
  // ImageShmManager h264_shm("h264_shm");
  if (yuyv_shm.create_and_init(20 * 1024 * 1024, 5 * 1024 * 1024) !=
          ShmStatus::Success) {
    std::cerr << "Failed to initialize shared memory" << std::endl;
    return 1;
  }

  // Initialize camera
  cv::VideoCapture cap(0);
  if (!cap.isOpened()) {
    std::cerr << "Failed to open camera" << std::endl;
    return 1;
  }
  cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
  cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
  cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));

  /*
  // Initialize FFmpeg H.264 encoder
  const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    std::cerr << "H.264 codec not found" << std::endl;
    cap.release();
    return 1;
  }

  AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx) {
    std::cerr << "Failed to allocate codec context" << std::endl;
    cap.release();
    return 1;
  }

  codec_ctx->width = 1920;
  codec_ctx->height = 1080;
  codec_ctx->time_base = {1, 30};
  codec_ctx->framerate = {30, 1};
  codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx->bit_rate = 4000000; // 4Mbps
  codec_ctx->gop_size = 10;
  codec_ctx->max_b_frames = 0;

  if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
    std::cerr << "Failed to open codec" << std::endl;
    avcodec_free_context(&codec_ctx);
    cap.release();
    return 1;
  }

  // Initialize swscale for YUYV to YUV420P
  SwsContext *sws_ctx = sws_getContext(1920, 1080, AV_PIX_FMT_YUYV422, 1920,
                                       1080, AV_PIX_FMT_YUV420P, SWS_BILINEAR,
                                       nullptr, nullptr, nullptr);
  if (!sws_ctx) {
    std::cerr << "Failed to initialize swscale" << std::endl;
    avcodec_free_context(&codec_ctx);
    cap.release();
    return 1;
  }

  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Failed to allocate frame" << std::endl;
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    cap.release();
    return 1;
  }

  frame->format = AV_PIX_FMT_YUV420P;
  frame->width = 1920;
  frame->height = 1080;
  if (av_frame_get_buffer(frame, 32) < 0) { // 使用32字节对齐以提高性能
    std::cerr << "Failed to allocate frame buffer" << std::endl;
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    cap.release();
    return 1;
  }

  AVPacket *packet = av_packet_alloc();
  if (!packet) {
    std::cerr << "Failed to allocate packet" << std::endl;
    av_frame_free(&frame);
    sws_freeContext(sws_ctx);
    avcodec_free_context(&codec_ctx);
    cap.release();
    return 1;
  }
  */

  uint64_t frame_version = 0;
  cv::Mat yuyv_frame;

  std::cout << "Starting video capture and encoding..." << std::endl;

  while (cap.read(yuyv_frame)) {
    if (yuyv_frame.empty()) {
      std::cerr << "Captured empty frame" << std::endl;
      continue;
    }

    // Write YUYV to shared memory
    if (yuyv_shm.write_image(yuyv_frame.data,
                             yuyv_frame.total() * yuyv_frame.elemSize(),
                             yuyv_frame.cols, yuyv_frame.rows, 2, frame_version,
                             ImageFormat::YUYV) != ShmStatus::Success) {
      std::cerr << "Failed to write YUYV to shared memory" << std::endl;
      continue;
    }

    /*
    // 确保frame数据是可写的
    if (av_frame_make_writable(frame) < 0) {
      std::cerr << "Failed to make frame writable" << std::endl;
      continue;
    }

    // Convert YUYV to YUV420P for H.264 encoding
    const uint8_t *src_data[4] = {yuyv_frame.data, nullptr, nullptr, nullptr};
    int src_linesize[4] = {static_cast<int>(yuyv_frame.step), 0, 0, 0};
    sws_scale(sws_ctx, src_data, src_linesize, 0, yuyv_frame.rows, frame->data,
              frame->linesize);
    frame->pts = frame_version;

    // Encode to H.264
    int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
      std::cerr << "Error sending frame to encoder: " << ret << std::endl;
      continue;
    }

    while (ret >= 0) {
      ret = avcodec_receive_packet(codec_ctx, packet);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }
      if (ret < 0) {
        std::cerr << "Error receiving packet from encoder: " << ret
                  << std::endl;
        break;
      }

      if (h264_shm.write_image(packet->data, packet->size, 1920, 1080, 0,
                               frame_version, ImageFormat::H264,
                               packet->flags & AV_PKT_FLAG_KEY ? 1 : 2) !=
          ShmStatus::Success) {
        std::cerr << "Failed to write H.264 to shared memory" << std::endl;
      }
      av_packet_unref(packet);
    }
    */

    frame_version++;

    // 添加退出条件，避免无限循环
    if (frame_version > 1000) { // 测试用，实际可以移除
      std::cout << "Captured 1000 frames, stopping..." << std::endl;
      break;
    }
  }

  /*
  // 发送flush frame以获取剩余的编码帧
  avcodec_send_frame(codec_ctx, nullptr);
  int ret = 0;
  while (ret >= 0) {
    ret = avcodec_receive_packet(codec_ctx, packet);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    }
    if (ret >= 0) {
      av_packet_unref(packet);
    }
  }
  */

  std::cout << "Cleaning up resources..." << std::endl;

  /*
  // Cleanup
  av_frame_free(&frame);
  av_packet_free(&packet);
  sws_freeContext(sws_ctx);
  avcodec_free_context(&codec_ctx);
  */
  cap.release();

  // 清理共享内存
  yuyv_shm.unmap_and_close();
  // h264_shm.unmap_and_close();
  yuyv_shm.unlink_shm();
  // h264_shm.unlink_shm();

  std::cout << "Video capture completed successfully." << std::endl;
  return 0;
}
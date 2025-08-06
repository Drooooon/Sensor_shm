#include "capture_control.h" // 添加这个头文件
#include "video_capture.h"
#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <signal.h>
#include <thread>

static VideoCaptureModule *capture_module = nullptr;
static bool show_preview = false;

void signalHandler(int signal) {
  if (capture_module) {
    std::cout << "\nShutting down...\n";
    capture_module->stop();
    delete capture_module;
    cv::destroyAllWindows();
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  int camera_index = 1;
  // 解析命令行参数
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--preview" || std::string(argv[i]) == "-p") {
      show_preview = true;
    } else {
      camera_index = std::atoi(argv[i]);
    }
  }

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  capture_module = new VideoCaptureModule();
  if (!capture_module->initialize(camera_index)) {
    std::cerr << "Failed to initialize camera " << camera_index << std::endl;
    return -1;
  }

  std::cout << "Starting video capture from camera " << camera_index;
  if (show_preview) {
    std::cout << " with preview window (1 FPS)";
  }
  std::cout << std::endl;

  capture_module->start();

  if (show_preview) {
    cv::namedWindow("Video Preview", cv::WINDOW_AUTOSIZE);

    while (show_preview) {
      // 使用共享内存控制器来获取图像数据
      CaptureControlWrapper control;
      if (control.initialize()) {
        // 从共享内存获取JPEG数据
        uint32_t jpeg_size = control.getJPEGSize();
        if (jpeg_size > 0) {
          unsigned char *jpeg_data = control.getJPEGData();
          if (jpeg_data) {
            // 将JPEG数据解码为OpenCV Mat
            std::vector<uchar> jpeg_buf(jpeg_data, jpeg_data + jpeg_size);
            cv::Mat frame = cv::imdecode(jpeg_buf, cv::IMREAD_COLOR);
            if (!frame.empty()) {
              cv::imshow("Video Preview", frame);
            }
          }
        }
      } else {
        std::cerr << "Failed to initialize shared memory for preview"
                  << std::endl;
      }

      // 检查是否有按键输入（非阻塞）
      int key = cv::waitKey(1) & 0xFF;
      if (key == 27 || key == 'q') {
        signalHandler(SIGINT);
        break;
      }

      // 等待1秒钟
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  if (!show_preview) {
    // 保持程序运行（无预览模式）
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  return 0;
}
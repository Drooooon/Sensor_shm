#include "image_shm_manager.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <cstring>
#include <thread>

// 全局变量用于信号处理
volatile bool g_running = true;

void signal_handler(int signum) {
    std::cout << "\n收到信号 " << signum << "，正在退出..." << std::endl;
    g_running = false;
}

// 简化的捕获进程（仅OpenCV，无FFmpeg编码）
int capture_process_simple() {
    std::cout << "启动简化捕获进程（仅YUYV，无H.264编码）..." << std::endl;
    
    // 初始化共享内存（仅需要YUYV）
    ImageShmManager yuyv_shm("yuyv_shm");
    if (yuyv_shm.create_and_init(12 * 1024 * 1024, 4 * 1024 * 1024) != ShmStatus::Success) {
        std::cerr << "无法初始化YUYV共享内存" << std::endl;
        return 1;
    }
    
    // 初始化摄像头
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "无法打开摄像头" << std::endl;
        return 1;
    }
    
    // 设置摄像头参数
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'));
    cap.set(cv::CAP_PROP_FPS, 30);
    
    uint64_t frame_version = 0;
    cv::Mat yuyv_frame;
    
    std::cout << "开始捕获视频（仅YUYV格式）..." << std::endl;
    
    while (g_running && cap.read(yuyv_frame)) {
        if (yuyv_frame.empty()) {
            std::cerr << "捕获到空帧" << std::endl;
            continue;
        }
        
        // 写入YUYV数据到共享内存
        if (yuyv_shm.write_image(yuyv_frame.data,
                                 yuyv_frame.total() * yuyv_frame.elemSize(),
                                 yuyv_frame.cols, yuyv_frame.rows, 2, 
                                 frame_version, ImageFormat::YUYV) != ShmStatus::Success) {
            std::cerr << "写入YUYV到共享内存失败" << std::endl;
            continue;
        }
        
        frame_version++;
        
        // 控制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
        
        // 测试用退出条件
        if (frame_version > 3000) {
            std::cout << "捕获了3000帧，停止..." << std::endl;
            break;
        }
    }
    
    std::cout << "捕获进程清理资源..." << std::endl;
    cap.release();
    yuyv_shm.unmap_and_close();
    yuyv_shm.unlink_shm();
    
    return 0;
}

// 读取和可视化进程
int reader_process() {
    std::cout << "启动读取进程..." << std::endl;
    
    // 初始化共享内存（仅YUYV）
    ImageShmManager yuyv_shm("yuyv_shm");
    
    // 等待共享内存创建
    sleep(2);
    
    if (yuyv_shm.open_and_map(12 * 1024 * 1024, 4 * 1024 * 1024) != ShmStatus::Success) {
        std::cerr << "读取进程：无法连接到YUYV共享内存" << std::endl;
        return 1;
    }
    
    // 创建OpenCV窗口
    cv::namedWindow("YUYV Preview", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("Stream Info", cv::WINDOW_AUTOSIZE);
    
    // 分配缓冲区
    const size_t max_buffer_size = 12 * 1024 * 1024;
    uint8_t* buffer = new uint8_t[max_buffer_size];
    
    uint64_t last_yuyv_version = 0;
    uint32_t frame_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    // 创建信息显示图像
    cv::Mat info_frame = cv::Mat::zeros(400, 600, CV_8UC3);
    
    while (g_running) {
        bool frame_updated = false;
        
        // 读取YUYV数据
        uint32_t width, height, channels;
        size_t data_size;
        uint64_t frame_version;
        ImageFormat format;
        uint8_t frame_type;
        
        if (yuyv_shm.read_image(buffer, max_buffer_size, &width, &height, 
                               &channels, &data_size, &frame_version, 
                               &format, &frame_type) == ShmStatus::Success) {
            if (frame_version > last_yuyv_version) {
                last_yuyv_version = frame_version;
                frame_count++;
                
                // 将YUYV转换为BGR进行显示
                cv::Mat yuyv_mat(height, width, CV_8UC2, buffer);
                cv::Mat bgr_mat;
                cv::cvtColor(yuyv_mat, bgr_mat, cv::COLOR_YUV2BGR_YUYV);
                
                // 缩放以适应显示
                cv::Mat display_mat;
                cv::resize(bgr_mat, display_mat, cv::Size(640, 360));
                
                cv::imshow("YUYV Preview", display_mat);
                frame_updated = true;
                
                // 更新信息显示
                info_frame = cv::Scalar(0, 0, 0);
                
                // 计算FPS
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    current_time - start_time).count();
                double fps = elapsed > 0 ? frame_count / (double)elapsed : 0;
                
                // 显示信息
                cv::putText(info_frame, "Simple YUYV Stream", cv::Point(20, 40), 
                           cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
                cv::putText(info_frame, "Frame: " + std::to_string(frame_version), 
                           cv::Point(20, 80), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                           cv::Scalar(255, 255, 255), 2);
                cv::putText(info_frame, "Size: " + std::to_string(data_size) + " bytes", 
                           cv::Point(20, 120), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                           cv::Scalar(255, 255, 255), 2);
                cv::putText(info_frame, "Resolution: " + std::to_string(width) + "x" + std::to_string(height), 
                           cv::Point(20, 160), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                           cv::Scalar(255, 255, 255), 2);
                cv::putText(info_frame, "FPS: " + std::to_string(fps), 
                           cv::Point(20, 200), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                           cv::Scalar(0, 255, 255), 2);
                cv::putText(info_frame, "Total Frames: " + std::to_string(frame_count), 
                           cv::Point(20, 240), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                           cv::Scalar(0, 255, 255), 2);
                cv::putText(info_frame, "Format: YUYV (No H.264)", 
                           cv::Point(20, 280), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                           cv::Scalar(255, 0, 255), 2);
                
                cv::imshow("Stream Info", info_frame);
            }
        }
        
        // 处理OpenCV窗口事件
        char key = cv::waitKey(1);
        if (key == 27 || key == 'q') { // ESC或q键退出
            g_running = false;
            break;
        }
        
        // 如果没有新帧，短暂休眠
        if (!frame_updated) {
            usleep(10000); // 10ms
        }
    }
    
    std::cout << "读取进程结束，共处理 " << frame_count << " 帧" << std::endl;
    
    // 清理
    delete[] buffer;
    cv::destroyAllWindows();
    
    return 0;
}

int main() {
    std::cout << "启动简化视频流测试程序（无FFmpeg）..." << std::endl;
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    pid_t capture_pid, reader_pid;
    
    // 启动捕获进程
    capture_pid = fork();
    if (capture_pid == 0) {
        // 子进程：运行简化捕获
        return capture_process_simple();
    } else if (capture_pid < 0) {
        std::cerr << "无法创建捕获进程" << std::endl;
        return 1;
    }
    
    // 启动读取进程
    reader_pid = fork();
    if (reader_pid == 0) {
        // 子进程：运行读取器
        return reader_process();
    } else if (reader_pid < 0) {
        std::cerr << "无法创建读取进程" << std::endl;
        kill(capture_pid, SIGTERM);
        return 1;
    }
    
    std::cout << "两个进程已启动：" << std::endl;
    std::cout << "  捕获进程 PID: " << capture_pid << std::endl;
    std::cout << "  读取进程 PID: " << reader_pid << std::endl;
    std::cout << "按Ctrl+C退出程序" << std::endl;
    
    // 主进程等待子进程
    int status;
    pid_t finished_pid;
    
    while (g_running && (finished_pid = waitpid(-1, &status, WNOHANG)) >= 0) {
        if (finished_pid > 0) {
            if (finished_pid == capture_pid) {
                std::cout << "捕获进程已退出" << std::endl;
                capture_pid = -1;
            } else if (finished_pid == reader_pid) {
                std::cout << "读取进程已退出" << std::endl;
                reader_pid = -1;
            }
            
            // 如果任一进程退出，终止另一个
            if (capture_pid > 0) {
                kill(capture_pid, SIGTERM);
            }
            if (reader_pid > 0) {
                kill(reader_pid, SIGTERM);
            }
            break;
        }
        sleep(1);
    }
    
    // 确保所有子进程都被终止
    if (capture_pid > 0) {
        std::cout << "终止捕获进程..." << std::endl;
        kill(capture_pid, SIGTERM);
        waitpid(capture_pid, &status, 0);
    }
    
    if (reader_pid > 0) {
        std::cout << "终止读取进程..." << std::endl;
        kill(reader_pid, SIGTERM);
        waitpid(reader_pid, &status, 0);
    }
    
    std::cout << "简化视频流测试程序结束" << std::endl;
    return 0;
}
#!/bin/bash

# 视频流测试运行脚本

echo "================================"
echo "视频流双进程测试脚本"
echo "================================"

# 检查是否在正确的目录
if [ ! -f "capture_process.cpp" ] || [ ! -f "test_video_streaming.cpp" ]; then
    echo "错误：请在 src/cpp/video 目录下运行此脚本"
    exit 1
fi

# 清理之前的编译结果
echo "清理之前的编译结果..."
make clean

# 检查依赖
echo "检查系统依赖..."
if ! pkg-config --exists opencv4; then
    echo "警告：未检测到 OpenCV，尝试安装依赖..."
    make install-deps
fi

# 编译程序
echo "编译程序..."
if ! make all; then
    echo "编译失败，请检查依赖是否正确安装"
    exit 1
fi

echo "编译成功！"

# 检查摄像头设备
echo "检查摄像头设备..."
if [ ! -e "/dev/video0" ]; then
    echo "警告：未检测到 /dev/video0 摄像头设备"
    echo "程序可能无法正常工作，但仍将尝试运行"
fi

# 清理可能存在的共享内存
echo "清理可能存在的共享内存..."
sudo rm -f /dev/shm/yuyv_shm /dev/shm/h264_shm 2>/dev/null

echo ""
echo "================================"
echo "启动测试程序"
echo "================================"
echo "程序将启动两个进程："
echo "1. 捕获进程：从摄像头捕获视频并写入共享内存"
echo "2. 显示进程：从共享内存读取数据并显示视频流"
echo ""
echo "控制说明："
echo "- 按 Ctrl+C 退出程序"
echo "- 在显示窗口中按 ESC 或 'q' 也可退出"
echo "================================"
echo ""

# 运行测试程序
./test_video_streaming

echo ""
echo "================================"
echo "测试程序结束"
echo "================================"

# 清理共享内存
echo "清理共享内存..."
sudo rm -f /dev/shm/yuyv_shm /dev/shm/h264_shm 2>/dev/null

echo "清理完成！"
# 视频流双进程测试系统

这个测试系统实现了双进程架构的视频流处理：
- **捕获进程**: 使用摄像头捕获视频，编码为H.264并写入共享内存
- **显示进程**: 从共享内存读取数据并实时显示视频流

## 系统架构

```
┌─────────────────┐    共享内存    ┌─────────────────┐
│   捕获进程      │ =============> │   显示进程      │
│ capture_process │                │ reader_process  │
│                 │                │                 │
│ ┌─────────────┐ │                │ ┌─────────────┐ │
│ │  摄像头捕获  │ │                │ │  YUYV显示   │ │
│ │  YUYV格式   │ │ ═══YUYV═══>   │ │  OpenCV窗口 │ │
│ └─────────────┘ │                │ └─────────────┘ │
│                 │                │                 │
│ ┌─────────────┐ │                │ ┌─────────────┐ │
│ │  H.264编码  │ │ ═══H.264═══>  │ │  流信息显示  │ │
│ │  FFmpeg     │ │                │ │  帧率/大小   │ │
│ └─────────────┘ │                │ └─────────────┘ │
└─────────────────┘                └─────────────────┘
```

## 文件说明

- `capture_process.cpp` - 摄像头捕获和编码程序
- `test_video_streaming.cpp` - 双进程测试主程序
- `image_shm_manager.h/cpp` - 图像共享内存管理器
- `Makefile` - 编译配置
- `run_test.sh` - 自动化测试脚本

## 依赖要求

### 系统依赖
```bash
sudo apt-get install -y \
    libopencv-dev \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    build-essential
```

### 硬件要求
- USB摄像头或内置摄像头 (`/dev/video0`)
- 支持YUYV格式的摄像头（大多数USB摄像头支持）

## 快速开始

### 1. 自动化运行（推荐）
```bash
cd /home/dron/SensorComm/src/cpp/video
chmod +x run_test.sh
./run_test.sh
```

### 2. 手动编译和运行
```bash
# 清理
make clean

# 编译
make all

# 运行测试
./test_video_streaming
```

## 程序功能

### 捕获进程功能
- 从摄像头捕获1920x1080 YUYV格式视频
- 将YUYV原始数据写入共享内存
- 使用FFmpeg将YUYV转换为YUV420P并编码为H.264
- 将H.264编码数据写入另一个共享内存区域
- 支持I帧和P帧标记

### 显示进程功能
- 从共享内存读取YUYV数据并转换为BGR显示
- 实时显示缩放后的视频预览（640x360）
- 显示H.264流信息：
  - 当前帧号
  - 数据大小
  - 帧类型（I帧/P帧）
  - 实时帧率
  - 总帧数

## 控制说明

- **退出程序**: 按 `Ctrl+C` 或在OpenCV窗口中按 `ESC` 或 `q`
- **窗口**: 
  - `YUYV Preview`: 实时视频预览
  - `H264 Info`: H.264流统计信息

## 共享内存

系统使用两个共享内存区域：
- `yuyv_shm`: 存储YUYV原始视频数据（12MB缓冲区）
- `h264_shm`: 存储H.264编码数据（3MB缓冲区）

## 故障排除

### 1. 摄像头问题
```bash
# 检查摄像头设备
ls /dev/video*

# 查看摄像头支持的格式
v4l2-ctl --device=/dev/video0 --list-formats-ext
```

### 2. 依赖问题
```bash
# 检查OpenCV
pkg-config --modversion opencv4

# 检查FFmpeg库
pkg-config --modversion libavcodec libavformat
```

### 3. 共享内存清理
```bash
# 手动清理共享内存
sudo rm -f /dev/shm/yuyv_shm /dev/shm/h264_shm

# 查看共享内存使用
ls -la /dev/shm/
```

### 4. 编译问题
```bash
# 详细编译信息
make clean
make VERBOSE=1
```

## 性能参数

- **视频分辨率**: 1920x1080
- **帧率**: 30 FPS
- **H.264比特率**: 4 Mbps
- **GOP大小**: 10帧
- **显示分辨率**: 640x360（缩放显示）

## 扩展功能

可以基于此系统扩展的功能：
- 多摄像头支持
- 不同编码格式（H.265、VP9等）
- 网络流传输
- 录制功能
- 实时图像处理算法集成

## 技术特点

- **零拷贝**: 使用共享内存实现进程间高效数据传输
- **实时性**: 低延迟的视频处理管道
- **模块化**: 捕获和显示进程独立，易于扩展
- **健壮性**: 完善的错误处理和资源清理
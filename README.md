# SensorComm-docker

**高性能传感器数据通信系统**

一个基于 C++ 的跨平台传感器数据采集与传输系统，专注于实时视频数据处理和共享内存通信。

## 🚀 项目简介

SensorComm-docker 是一个企业级的传感器通信解决方案，采用生产者-消费者模式，通过共享内存实现高效的视频数据传输。系统支持多种视频格式，具有低延迟、高吞吐量的特点，适用于实时视频监控、计算机视觉应用和工业自动化场景。

### 核心特性

- 🎥 **多格式视频支持**: 支持 YUYV、MJPEG 等主流格式
- ⚡ **低延迟传输**: 基于共享内存的零拷贝数据传输
- 🔧 **V4L2 兼容**: 原生支持 Linux V4L2 视频设备
- 🏭 **工厂模式设计**: 灵活的组件创建和扩展机制
- 📊 **实时监控**: 内置性能监控和 FPS 统计
- 🛡️ **异常安全**: RAII 资源管理和优雅错误处理
- 🔄 **动态格式切换**: 运行时无缝切换视频格式

## 🏗️ 系统架构

```
┌─────────────────┐    共享内存     ┌─────────────────┐
│   Producer      │◄──────────────►│   Consumer      │
│                 │                 │                 │
│ ┌─────────────┐ │                 │ ┌─────────────┐ │
│ │ V4L2 Capture│ │                 │ │GUI Display  │ │
│ │   Engine    │ │                 │ │   Engine    │ │
│ └─────────────┘ │                 │ └─────────────┘ │
│ ┌─────────────┐ │                 │ ┌─────────────┐ │
│ │Shared Memory│ │                 │ │Video Decoder│ │
│ │  Manager    │ │                 │ │  (Factory)  │ │
│ └─────────────┘ │                 │ └─────────────┘ │
└─────────────────┘                 └─────────────────┘
```

### 主要组件

- **生产者进程** (`producer_process`): 从 V4L2 设备捕获视频数据
- **消费者进程** (`consumer_process/consumer_gui`): 读取并显示视频数据
- **共享内存管理器** (`ImageShmManager`): 高效的内存映射和数据传输
- **格式解码器** (`YuyvDecoder`, `MjpgDecoder`): 多种视频格式解码支持
- **配置管理器** (`ConfigManager`): 统一的配置文件管理

## 📋 系统要求

### 运行环境
- **操作系统**: Linux (Ubuntu 18.04+, CentOS 7+ 或类似发行版)
- **编译器**: GCC 7.0+ 或 Clang 6.0+ (支持 C++14)
- **内存**: 至少 256MB 可用内存
- **硬件**: 支持 V4L2 的视频设备 (如 USB 摄像头)

### 依赖库
- **OpenCV** (>= 4.0): 图像处理和显示
- **nlohmann/json** (>= 3.7): JSON 配置文件解析
- **V4L2 开发库**: Linux 视频设备接口
- **POSIX 共享内存**: 系统级 IPC 支持

## 🛠️ 编译安装

### 1. 克隆项目
```bash
git clone https://github.com/your-org/SensorComm-docker.git
cd SensorComm-docker
```

### 2. 安装依赖 (Ubuntu/Debian)
```bash
# 安装基础开发工具
sudo apt update
sudo apt install build-essential cmake pkg-config

# 安装 OpenCV
sudo apt install libopencv-dev libopencv-contrib-dev

# 安装 V4L2 开发库
sudo apt install libv4l-dev v4l-utils

# 验证摄像头设备
ls /dev/video*
v4l2-ctl --list-devices
```

### 3. 编译项目
```bash
cd src/cpp
make clean
make all

# 编译成功后可执行文件将位于:
# - build/bin/producer_process
# - build/bin/consumer_process  
# - build/bin/consumer_gui
```

### 编译选项
- `make debug`: 编译调试版本 (包含调试符号)
- `make release`: 编译优化版本 (生产环境)
- `make clean`: 清理编译文件

## ⚙️ 配置说明

### 视频配置 (`config/videoConfig.json`)
```json
{
  "v4l2_capture": {
    "device_path": "/dev/video0",    // 视频设备路径
    "width": 1280,                   // 视频宽度
    "height": 720,                   // 视频高度  
    "format": "YUYV",               // 像素格式 (YUYV/MJPG)
    "buffer_count": 4               // V4L2 缓冲区数量
  }
}
```

### 共享内存配置 (`config/shmConfig.json`)
```json
{
  "shared_memory": {
    "name": "yuyv_shm",             // 共享内存名称
    "total_size_mb": 32,            // 总内存大小 (MB)
    "buffer_size_mb": 10,           // 单个缓冲区大小 (MB)
    "buffer_count": 3               // 缓冲区数量
  }
}
```

### 配置参数说明

| 参数 | 说明 | 推荐值 |
|------|------|---------|
| `device_path` | V4L2设备路径 | `/dev/video0` |
| `width/height` | 视频分辨率 | `1280x720` (HD), `640x480` (VGA) |
| `format` | 像素格式 | `YUYV` (未压缩), `MJPG` (压缩) |
| `total_size_mb` | 共享内存总大小 | 32MB (可根据分辨率调整) |
| `buffer_count` | 环形缓冲区数量 | 3-4 (平衡延迟和稳定性) |

## 🚀 使用指南

### 快速启动

1. **启动生产者进程** (在终端1中):
```bash
cd src/cpp/video
./build/bin/producer_process
```

2. **启动消费者GUI** (在终端2中):
```bash
cd src/cpp/video  
./build/bin/consumer_gui
```

### 命令行选项
```bash
# 使用自定义配置文件
./producer_process --video-config=/path/to/video.json --shm-config=/path/to/shm.json

# 启用详细日志
./consumer_gui --verbose --log-fps

# 保存视频帧到文件
./consumer_process --save-frames --max-frames=100
```

### 测试和诊断

运行设备兼容性测试:
```bash
cd src/cpp/video/test
chmod +x test_video.sh
./test_video.sh
```

该脚本将检查:
- 摄像头设备可用性
- 支持的视频格式和分辨率
- USB 总线带宽和性能
- 系统资源使用情况

## 📊 性能优化

### 系统调优建议

1. **内存优化**:
   ```bash
   # 增加共享内存限制
   echo 'kernel.shmmax = 134217728' >> /etc/sysctl.conf  # 128MB
   echo 'kernel.shmall = 32768' >> /etc/sysctl.conf
   sysctl -p
   ```

2. **USB 设备优化**:
   ```bash
   # 增加 USB 缓冲区大小
   echo 16 > /sys/module/usbcore/parameters/usbfs_memory_mb
   ```

3. **实时优先级** (可选):
   ```bash
   # 以更高优先级运行
   sudo nice -n -10 ./producer_process
   ```

### 性能监控

系统提供实时性能统计:
- **FPS (帧率)**: 每2秒输出当前捕获/显示帧率
- **延迟监控**: 帧时间戳跟踪和延迟计算
- **内存使用**: 共享内存缓冲区利用率

## 🔧 开发指南

### 代码结构
```
src/cpp/
├── common/
│   ├── ipc/              # 进程间通信 (共享内存)
│   └── json/             # JSON 解析库
├── config/               # 配置管理和工厂模式
├── video/
│   ├── formats/          # 视频格式处理 (捕获/解码)
│   ├── test/            # 测试程序
│   └── *.cpp            # 主要业务逻辑
└── Makefile
```

### 扩展新格式

1. **添加新的解码器**:
```cpp
// 继承 IDecoder 接口
class H264Decoder : public IDecoder {
public:
    cv::Mat decode(const uint8_t* data, const ImageHeader& header) override;
};
```

2. **在工厂中注册**:
```cpp
// factory.cpp
case ImageFormat::H264:
    return std::make_unique<H264Decoder>();
```

### 添加新的捕获源

1. **实现 ICapture 接口**:
```cpp
class NetworkCapture : public ICapture {
    void start() override;
    void stop() override;
    bool capture(CapturedFrame& frame, std::atomic<bool>& running) override;
};
```

## 🐛 故障排除

### 常见问题

**Q: 找不到摄像头设备**
```bash
# 检查设备权限
ls -l /dev/video*
sudo chmod 666 /dev/video0

# 检查设备是否被占用
lsof /dev/video0
```

**Q: 共享内存创建失败**
```bash
# 检查共享内存限制
ipcs -m
# 清理残留的共享内存
ipcrm -M <key>
```

**Q: 编译错误**
```bash
# 确保安装了所有依赖
pkg-config --libs opencv4
# 检查编译器版本
gcc --version
```

**Q: 视频帧率过低**
- 降低分辨率 (如从 1280x720 到 640x480)
- 检查 USB 总线带宽 (`lsusb -t`)
- 增加缓冲区数量
- 确保 CPU 负载不过高
- 更换图片格式

### 日志分析

系统日志关键信息:
```
Producer: Started capture stream.          # 捕获启动成功
Producer: FPS: 29.5, Frames: 590          # 性能统计
ConsumerGUI: Format changed from 0 to 1    # 动态格式切换
ConsumerGUI: Display FPS: 30.0             # 显示帧率
```


### 代码规范
- 遵循 Google C++ 风格指南
- 使用 Doxygen 格式注释
- 确保通过所有测试用例


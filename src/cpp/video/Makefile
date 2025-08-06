# Makefile for video streaming test (FFmpeg + OpenCV + libx264/libx265 手动集成版本)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# 项目根目录（请根据实际路径调整）
PROJECT_ROOT := $(realpath ../../..)
FFMPEG_DIR := $(PROJECT_ROOT)/third_party/ffmpeg

# --- 头文件路径 ---
INCLUDES = -I../common -I$(FFMPEG_DIR)/include
INCLUDES += $(shell pkg-config --cflags opencv4)

# --- 链接库路径和库 ---
LIBS_OPENCV = $(shell pkg-config --libs opencv4)

# FFmpeg静态库（使用 --whole-archive 确保所有符号被包含）
LIBS_FFMPEG_STATIC = -Wl,--whole-archive \
                     $(FFMPEG_DIR)/lib/libavformat.a \
                     $(FFMPEG_DIR)/lib/libavcodec.a \
                     $(FFMPEG_DIR)/lib/libswscale.a \
                     $(FFMPEG_DIR)/lib/libswresample.a \
                     $(FFMPEG_DIR)/lib/libavutil.a \
                     $(FFMPEG_DIR)/lib/libavfilter.a \
                     -Wl,--no-whole-archive

# FFmpeg的依赖库（基于pkg-config信息）
LIBS_FFMPEG_DEPS = -lx264 -llzma -latomic

# 系统依赖库，放在最后，确保链接完整
LIBS_EXTRA = -lrt -lpthread -lm -lz -ldl

# 完整版本链接库：OpenCV + FFmpeg静态库 + FFmpeg依赖 + 系统库
LIBS_FULL = $(LIBS_OPENCV) $(LIBS_FFMPEG_STATIC) $(LIBS_FFMPEG_DEPS) $(LIBS_EXTRA)

# 简化版本，仅OpenCV和基础系统库
LIBS_SIMPLE = $(LIBS_OPENCV) -lrt -lpthread

# --- 源文件 ---
COMMON_SRCS = ../common/shm_manager.cpp
VIDEO_SRCS = image_shm_manager.cpp
CAPTURE_SRCS = capture_process.cpp
PRODUCER_SRCS = producer_process.cpp
CONSUMER_SRCS = consumer_process.cpp
TEST_SRCS = test_video_streaming.cpp
TEST_SIMPLE_SRCS = test_video_streaming_simple.cpp

# --- 目标文件 ---
COMMON_OBJS = $(patsubst %.cpp, %.o, $(COMMON_SRCS))
VIDEO_OBJS = $(patsubst %.cpp, %.o, $(VIDEO_SRCS))
CAPTURE_OBJS = $(patsubst %.cpp, %.o, $(CAPTURE_SRCS))
PRODUCER_OBJS = $(patsubst %.cpp, %.o, $(PRODUCER_SRCS))
CONSUMER_OBJS = $(patsubst %.cpp, %.o, $(CONSUMER_SRCS))
TEST_OBJS = $(patsubst %.cpp, %.o, $(TEST_SRCS))
TEST_SIMPLE_OBJS = $(patsubst %.cpp, %.o, $(TEST_SIMPLE_SRCS))

.PHONY: all simple clean test

# 默认全部编译
all: test_video_streaming_simple capture_process producer_process consumer_process test_video_streaming camera_test

# 测试目标
test: producer_process consumer_process
	@echo "Test executables built successfully!"
	@echo "Run './test_streaming.sh' to start the test"

# 简化版本（仅OpenCV）
test_video_streaming_simple: $(COMMON_OBJS) $(VIDEO_OBJS) $(TEST_SIMPLE_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS_SIMPLE)

# 简化版本（仅OpenCV，不使用FFmpeg）
capture_process: $(COMMON_OBJS) $(VIDEO_OBJS) $(CAPTURE_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS_SIMPLE)

# 生产者进程（仅OpenCV）
producer_process: $(COMMON_OBJS) $(VIDEO_OBJS) $(PRODUCER_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS_SIMPLE)

# 消费者进程（仅OpenCV）
consumer_process: $(COMMON_OBJS) $(VIDEO_OBJS) $(CONSUMER_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS_SIMPLE)

# 摄像头测试工具
camera_test: camera_test.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS_SIMPLE)

# 完整版本测试程序
test_video_streaming: $(COMMON_OBJS) $(VIDEO_OBJS) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS_FULL)

# 编译规则（C++源文件 -> 目标文件）
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

../common/%.o: ../common/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 清理生成的目标文件和可执行文件
clean:
	rm -f *.o ../common/*.o capture_process test_video_streaming test_video_streaming_simple producer_process consumer_process producer_process consumer_process

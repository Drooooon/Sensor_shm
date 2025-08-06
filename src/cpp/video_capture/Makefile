# 编译配置
CXX = g++
CXXFLAGS = -std=c++14 -pthread -O2 -fPIC
OPENCV_FLAGS = $(shell pkg-config --cflags --libs opencv4)
INCLUDES = -Isrc/cpp

# 目标文件
CAPTURE_TARGET = build/capture_main
PYTHON_EXT_DIR = src/python

# 源文件 - 添加 capture_control.cpp
CAPTURE_SOURCES = src/cpp/capture_main.cpp src/cpp/video_capture.cpp src/cpp/capture_control.cpp
CONTROL_SOURCES = src/cpp/capture_control.cpp

.PHONY: all capture python clean install deps

all: capture python

# 创建build目录
build:
	mkdir -p build

# 编译摄像头采集程序
capture: build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CAPTURE_SOURCES) $(OPENCV_FLAGS) -lrt -o $(CAPTURE_TARGET)

# 编译Python扩展
python: 
	cd $(PYTHON_EXT_DIR) && \
	python setup.py build_ext --inplace

# 安装Python依赖
install:
	pip install -r requirements.txt

# 安装系统依赖
deps:
	@echo "Installing system dependencies..."
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y libopencv-dev python3-dev; \
	elif command -v yum >/dev/null 2>&1; then \
		sudo yum install -y opencv-devel python3-devel; \
	elif command -v pacman >/dev/null 2>&1; then \
		sudo pacman -S opencv python; \
	else \
		echo "Please install OpenCV development libraries manually"; \
	fi

# 清理编译文件
clean:
	rm -rf build/
	cd $(PYTHON_EXT_DIR) && rm -rf build/ *.so *.egg-info/
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -delete

# 测试编译
test-compile: capture
	@echo "Testing capture program..."
	@if [ -f $(CAPTURE_TARGET) ]; then \
		echo "✓ Capture program compiled successfully"; \
	else \
		echo "✗ Capture program compilation failed"; \
		exit 1; \
	fi

# 运行采集程序
run-capture:
	@if [ ! -f $(CAPTURE_TARGET) ]; then \
		echo "Building capture program first..."; \
		$(MAKE) capture; \
	fi
	./$(CAPTURE_TARGET)

# 运行Web服务器
run-server:
	cd src/python && python video_server.py

# 完整测试
test: test-compile
	@echo "启动采集程序..."
	./$(CAPTURE_TARGET) &
	sleep 2
	@echo "启动Web服务器..."
	cd src/python && python video_server.py

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all         - Build both capture program and Python extension"
	@echo "  capture     - Build only the capture program"
	@echo "  python      - Build only the Python extension"
	@echo "  install     - Install Python dependencies"
	@echo "  deps        - Install system dependencies"
	@echo "  clean       - Clean build files"
	@echo "  test-compile - Test if compilation works"
	@echo "  run-capture - Run the capture program"
	@echo "  run-server  - Run the web server"
	@echo "  help        - Show this help message"
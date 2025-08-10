# ==========================================================
#         src/cpp/Makefile (最终链接修正版)
# ==========================================================

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -g -pthread

# --- 1. 路径和目录定义 ---
BUILD_DIR = video/build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# --- 2. 头文件和链接库 ---
INCLUDES = -I.
INCLUDES += $(shell pkg-config --cflags opencv4)
LIBS = $(shell pkg-config --libs opencv4) -lrt

# --- 3. 源文件列表 (明确分离库和应用) ---
# a. 库文件源文件 (没有 main 函数)
LIB_SRCS = \
    common/ipc/shm_manager.cpp \
    config/config_manager.cpp \
    video/image_shm_manager.cpp \
    video/formats/yuyv_processor.cpp

# b. 主程序源文件 (每个都有 main 函数)
PRODUCER_APP_SRC = video/test/producer_process.cpp
CONSUMER_APP_SRC = video/test/consumer_process.cpp
CONSUMER_GUI_APP_SRC = video/test/consumer_gui.cpp

# --- 4. 自动化生成目标文件 (.o) ---
# $(notdir ...) 只取文件名, $(...:.cpp=.o) 替换后缀
LIB_OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(LIB_SRCS:.cpp=.o)))

PRODUCER_OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(PRODUCER_APP_SRC:.cpp=.o)))
CONSUMER_OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(CONSUMER_APP_SRC:.cpp=.o)))
CONSUMER_GUI_OBJ = $(addprefix $(OBJ_DIR)/, $(notdir $(CONSUMER_GUI_APP_SRC:.cpp=.o)))

# --- 5. 定义最终的可执行文件目标 ---
PRODUCER_EXEC = $(BIN_DIR)/producer_process
CONSUMER_EXEC = $(BIN_DIR)/consumer_process
CONSUMER_GUI_EXEC = $(BIN_DIR)/consumer_gui
EXECS = $(PRODUCER_EXEC) $(CONSUMER_EXEC) $(CONSUMER_GUI_EXEC)

# --- 6. 核心构建规则 ---
.PHONY: all clean run help

all: $(EXECS)

# *** 关键修复：精确的链接规则 ***
$(PRODUCER_EXEC): $(PRODUCER_OBJ) $(LIB_OBJS)
	@echo "Linking $@..."
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo "✅ Built executable: $@"

$(CONSUMER_EXEC): $(CONSUMER_OBJ) $(LIB_OBJS)
	@echo "Linking $@..."
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo "✅ Built executable: $@"

$(CONSUMER_GUI_EXEC): $(CONSUMER_GUI_OBJ) $(LIB_OBJS)
	@echo "Linking $@..."
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo "✅ Built executable: $@"

# 通用编译规则
# vpath 告诉 make 去哪里寻找源文件
vpath %.cpp common/ipc config video video/formats video/test

$(OBJ_DIR)/%.o: %.cpp
	@echo "Compiling $<..."
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# --- 7. 运行、清理和帮助 ---
run: all
	@echo "Running test script..."
	./video/test/test_streaming.sh

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "✅ Clean completed"

help:
	@echo "🚀 SensorComm C++ Build System"
	@echo "==============================="
	@echo "This Makefile is run from the 'src/cpp/' directory."
	@echo ""
	@echo "Usage:"
	@echo "  make        - Build all executables defined in '$(APP_DIR)'"
	@echo "  make run    - Build all and then run the main test script"
	@echo "  make clean  - Remove the entire build directory ('$(BUILD_DIR)')"
	@echo ""
	@echo "Detected Executables to be built:"
	@echo "  $(notdir $(EXECS))"
	@echo ""
	@echo "Output location:"
	@echo "  Executables: $(BIN_DIR)"
	@echo "  Object files: $(OBJ_DIR)"
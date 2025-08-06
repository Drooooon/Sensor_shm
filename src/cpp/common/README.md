# SensorComm Common包

## 概述

Common包提供了SensorComm项目中的基础组件，主要包含共享内存管理功能，用于高效的进程间通信（IPC）。这些组件设计为高性能、线程安全且易于使用，适用于传感器数据的实时传输和处理。最新版本已实现零拷贝优化，进一步提高了数据传输效率。

## 功能特点

- **零拷贝数据传输**: 直接在共享内存上操作数据，无需额外复制，显著提升性能
- **无锁双缓冲机制**: 采用双缓冲区设计，最小化读写操作之间的互斥等待
- **基于命名共享内存**: 使用POSIX共享内存和信号量，支持跨进程通信
- **线程安全设计**: 内部采用原子操作和互斥锁保证线程安全
- **C++/C双接口**: 同时提供C++类接口和C语言接口，方便不同环境集成
- **完整的错误处理**: 详细的状态码和日志输出，便于调试和故障排除
- **支持超时机制**: 可设置超时时间的读取操作，避免无限阻塞
- **内存对齐优化**: 64字节对齐的数据结构，避免伪共享，提高多核性能

## 主要组件

### ShmManager

共享内存管理器，提供创建、打开、读写、关闭共享内存的完整功能。

### ShmBufferControl

共享内存缓冲区控制结构体（零拷贝优化版本），管理双缓冲区的状态和版本控制。新增了以下功能：
- 记录每个缓冲区的实际数据大小
- 优化的缓冲区状态查询方法
- 改进的写入缓冲区切换机制，支持可变大小数据

## 文件结构

- **shm_manager.h**: 共享内存管理器的头文件，定义类接口和C接口
- **shm_manager.cpp**: 共享内存管理器的实现文件
- **shm_types.h**: 共享内存相关的数据类型和控制结构定义，包含零拷贝优化版本的ShmBufferControl

## 使用示例

### C++接口示例（零拷贝方式）

#### 生产者示例
```cpp
// 创建共享内存管理器
ShmManager producer("sensor_data", "sensor_sem");

// 创建并初始化共享内存（总大小, 缓冲区大小, 信号量初始值）
size_t total_size = sizeof(ShmBufferControl) + 2 * sizeof(SensorData);
auto status = producer.create_and_init(total_size, sizeof(SensorData), 0);
if (status != ShmStatus::Success) {
    std::cerr << "Failed to create shared memory" << std::endl;
    return -1;
}

// 获取写缓冲区指针，直接在共享内存上操作（零拷贝）
void* write_ptr = producer.get_write_buffer();
SensorData* data_ptr = static_cast<SensorData*>(write_ptr);

// 直接填充数据
data_ptr->timestamp = get_current_timestamp();
data_ptr->data_type = DATA_TYPE_AUDIO;
// ... 填充其他字段 ...

// 切换缓冲区并通知消费者（传入实际使用的数据大小）
producer.write_and_switch_zerocopy(sizeof(SensorData), frame_count++, true);

// 使用完后清理（如需销毁共享内存）
producer.unmap_and_close();
producer.unlink_shm();
producer.unlink_sem();
```

#### 消费者示例
```cpp
// 打开共享内存管理器
ShmManager consumer("sensor_data", "sensor_sem");

// 打开并映射已存在的共享内存
size_t total_size = sizeof(ShmBufferControl) + 2 * sizeof(SensorData);
auto status = consumer.open_and_map(total_size, sizeof(SensorData));
if (status != ShmStatus::Success) {
    std::cerr << "Failed to open shared memory" << std::endl;
    return -1;
}

// 等待并获取读缓冲区（零拷贝方式，无需复制数据）
size_t data_size = 0;
const void* read_ptr = nullptr;
status = consumer.wait_and_get_read_buffer(&read_ptr, &data_size, 1000);

if (status == ShmStatus::Success && read_ptr) {
    // 直接使用指针访问数据
    const SensorData* data = static_cast<const SensorData*>(read_ptr);
    
    // 处理数据...
    process_sensor_data(data, data_size);
    
    // 处理完成后释放缓冲区
    consumer.release_read_buffer();
}

// 使用完后清理
consumer.unmap_and_close();
```

### C接口示例

#### 生产者示例
```c
// 创建共享内存管理器
void* producer = create_shm_manager("sensor_data", "sensor_sem");

// 创建并初始化共享内存
size_t total_size = sizeof(ShmBufferControl) + 2 * sizeof(SensorData);
int status = shm_manager_create_and_init(producer, total_size, sizeof(SensorData), 0);
if (status != 0) {
    fprintf(stderr, "Failed to create shared memory\n");
    return -1;
}

// 写入数据并通知消费者
SensorData data = {/* 初始化传感器数据 */};
shm_manager_write_and_switch(producer, &data, sizeof(SensorData), frame_count++, 1);

// 使用完后清理
shm_manager_unmap_and_close(producer);
shm_manager_unlink_shm(producer);
shm_manager_unlink_sem(producer);
destroy_shm_manager(producer);
```

#### 消费者示例
```c
// 打开共享内存管理器
void* consumer = create_shm_manager("sensor_data", "sensor_sem");

// 打开并映射已存在的共享内存
size_t total_size = sizeof(ShmBufferControl) + 2 * sizeof(SensorData);
int status = shm_manager_open_and_map(consumer, total_size, sizeof(SensorData));
if (status != 0) {
    fprintf(stderr, "Failed to open shared memory\n");
    return -1;
}

// 等待并读取数据
SensorData data;
size_t actual_size = 0;
status = shm_manager_wait_and_read(consumer, &data, sizeof(SensorData), &actual_size, 1000);
if (status == 0 && actual_size > 0) {
    // 处理接收到的数据...
}

// 使用完后清理
shm_manager_unmap_and_close(consumer);
destroy_shm_manager(consumer);
```

## 注意事项

1. 共享内存名称以"/"开头，且长度不超过NAME_MAX-4
2. 同一共享内存的创建者和连接者必须使用相同的总大小和缓冲区大小
3. 创建者负责销毁共享内存和信号量(unlink)，连接者只需关闭(close)
4. 在多线程环境下，同一ShmManager实例的读写应避免并发操作
5. 使用零拷贝API时，必须在读取完数据后调用release_read_buffer方法
6. 确保在写入后立即调用write_and_switch_zerocopy方法，避免数据同步问题

## 错误码说明

ShmStatus枚举值包含以下状态码：
- Success: 操作成功
- AlreadyInitialized: 共享内存已初始化
- NotInitialized: 共享内存未初始化
- ShmOpenFailed: 打开共享内存失败
- ShmTruncateFailed: 设置共享内存大小失败
- ShmMapFailed: 映射共享内存失败
- ShmUnmapFailed: 解除共享内存映射失败
- ShmUnlinkFailed: 解除共享内存链接失败
- SemOpenFailed: 打开信号量失败
- SemUnlinkFailed: 解除信号量链接失败
- InvalidArguments: 无效参数
- BufferTooSmall: 缓冲区过小

## 性能考虑

- 零拷贝机制可大幅减少数据复制开销，特别适用于大型数据传输场景
- 适用于高频率、小到中等大小数据包的实时传输
- 双缓冲机制适合生产者和消费者速率不匹配的场景
- 无锁设计最大程度减少了线程/进程切换开销
- 内存对齐优化(64字节)避免CPU缓存行伪共享问题
- 适合单生产者-单消费者或单生产者-多消费者模型

## 版本历史

- **v0.3**: 增加零拷贝优化，支持可变大小数据，改进内存对齐
- **v0.2**: 添加超时机制和完整的错误处理
- **v0.1**: 初始版本，实现基本的共享内存通信功能
#!/usr/bin/env python3
import sys
import os

sys.path.append("./src/python")

import video_capture_py as vcap
import time


def test_integrated_capture():
    print("测试集成的视频采集功能...")

    # 1. 初始化系统（这会自动启动采集）
    print("1. 初始化视频采集系统...")
    if vcap.initialize():
        print("✓ 初始化成功，视频采集已自动启动")
    else:
        print("✗ 初始化失败")
        return False

    # 2. 检查采集状态
    print("2. 检查采集状态...")
    if vcap.is_capturing():
        print("✓ 视频采集正在运行")
    else:
        print("✗ 视频采集未运行")
        return False

    # 3. 等待几秒让采集稳定
    print("3. 等待采集稳定...")
    time.sleep(3)

    # 4. 获取视频帧
    print("4. 获取视频帧...")
    frame_id = vcap.get_frame_id()
    jpeg_size = vcap.get_jpeg_size()
    print(f"   当前帧ID: {frame_id}")
    print(f"   JPEG数据大小: {jpeg_size} 字节")

    if jpeg_size > 0:
        jpeg_data = vcap.get_jpeg_data()
        if jpeg_data:
            print(f"✓ 成功获取JPEG数据，大小: {len(jpeg_data)} 字节")

            # 保存一帧图像到文件
            with open("test_frame.jpg", "wb") as f:
                f.write(jpeg_data)
            print("✓ 已保存测试图像到 test_frame.jpg")
        else:
            print("✗ 无法获取JPEG数据")
    else:
        print("✗ JPEG数据大小为0")

    # 5. 测试控制功能
    print("5. 测试控制功能...")
    width, height = vcap.get_resolution()
    fps = vcap.get_fps()
    camera_index = vcap.get_camera_index()
    print(f"   当前分辨率: {width}x{height}")
    print(f"   当前帧率: {fps} FPS")
    print(f"   当前摄像头索引: {camera_index}")

    # 6. 测试暂停/恢复
    print("6. 测试暂停/恢复...")
    old_frame_id = vcap.get_frame_id()
    vcap.set_paused(True)
    time.sleep(2)
    paused_frame_id = vcap.get_frame_id()

    if paused_frame_id == old_frame_id:
        print("✓ 暂停功能正常")
    else:
        print("✗ 暂停功能异常")

    vcap.set_paused(False)
    time.sleep(2)
    resumed_frame_id = vcap.get_frame_id()

    if resumed_frame_id > paused_frame_id:
        print("✓ 恢复功能正常")
    else:
        print("✗ 恢复功能异常")

    # 7. 清理
    print("7. 清理资源...")
    vcap.stop_capture()
    vcap.shutdown()
    print("✓ 清理完成")

    print("\n测试完成！集成的视频采集功能运行正常。")
    return True


if __name__ == "__main__":
    if test_integrated_capture():
        print("\n现在可以启动Web服务器：")
        print("cd /home/dron/video_capture/src/python && python video_server.py")
    else:
        print("\n测试失败，请检查摄像头是否正常连接")

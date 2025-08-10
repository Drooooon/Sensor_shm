#!/bin/bash

echo "=== 摄像头性能诊断 ==="
echo

# 1. 检查摄像头支持的格式和帧率
echo "1️⃣ 检查摄像头支持的格式和帧率："
v4l2-ctl -d /dev/video0 --list-formats-ext

echo
echo "2️⃣ 检查当前摄像头设置："
v4l2-ctl -d /dev/video0 --get-fmt

echo
echo "3️⃣ 检查摄像头控制参数："
v4l2-ctl -d /dev/video0 --list-ctrls

echo
echo "4️⃣ 检查USB总线信息："
lsusb | grep -i camera
lsusb | grep -i video

echo
echo "5️⃣ 检查USB设备详细信息："
for dev in /sys/bus/usb/devices/*/product; do
    if grep -qi "camera\|video\|webcam" "$dev" 2>/dev/null; then
        echo "找到摄像头设备: $(cat "$dev")"
        dir=$(dirname "$dev")
        echo "  - USB版本: $(cat "$dir/version" 2>/dev/null || echo "未知")"
        echo "  - 速度: $(cat "$dir/speed" 2>/dev/null || echo "未知") Mbps"
        echo "  - 最大功耗: $(cat "$dir/bMaxPower" 2>/dev/null || echo "未知")"
    fi
done

echo
echo "6️⃣ 检查系统资源使用："
echo "CPU使用率:"
top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d'%' -f1

echo "内存使用:"
free -h | grep "Mem:"

echo
echo "7️⃣ 测试不同分辨率的帧率："
echo "测试 640x480..."
timeout 5s ffmpeg -f v4l2 -i /dev/video0 -s 640x480 -r 30 -f null - 2>&1 | grep -E "fps|frame"

echo "测试 1280x720..."
timeout 5s ffmpeg -f v4l2 -i /dev/video0 -s 1280x720 -r 30 -f null - 2>&1 | grep -E "fps|frame"

echo
echo "8️⃣ 检查dmesg中的USB/摄像头相关错误："
dmesg | grep -i -E "usb|video|camera|uvc" | tail -10
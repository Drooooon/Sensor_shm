from setuptools import setup, Extension
import os
import subprocess


# 获取OpenCV编译标志
def get_opencv_flags():
    try:
        # 使用pkg-config获取OpenCV编译标志
        cflags = (
            subprocess.check_output(["pkg-config", "--cflags", "opencv4"])
            .decode()
            .strip()
            .split()
        )
        libs = (
            subprocess.check_output(["pkg-config", "--libs", "opencv4"])
            .decode()
            .strip()
            .split()
        )

        # 解析include目录
        include_dirs = []
        for flag in cflags:
            if flag.startswith("-I"):
                include_dirs.append(flag[2:])

        # 解析库目录和库名
        library_dirs = []
        libraries = []
        for flag in libs:
            if flag.startswith("-L"):
                library_dirs.append(flag[2:])
            elif flag.startswith("-l"):
                libraries.append(flag[2:])

        return include_dirs, library_dirs, libraries
    except:
        # 如果pkg-config失败，使用默认路径
        return (
            ["/usr/include/opencv4"],
            [],
            [
                "opencv_core",
                "opencv_imgproc",
                "opencv_imgcodecs",
                "opencv_videoio",
                "opencv_highgui",
            ],
        )


# 获取绝对路径
current_dir = os.path.dirname(os.path.abspath(__file__))
cpp_dir = os.path.abspath(os.path.join(current_dir, "..", "cpp"))

# 获取OpenCV编译标志
opencv_includes, opencv_lib_dirs, opencv_libs = get_opencv_flags()

# 源文件（添加video_capture.cpp）
sources = [
    os.path.join(cpp_dir, "python_extension.cpp"),
    os.path.join(cpp_dir, "capture_control.cpp"),
    os.path.join(cpp_dir, "video_capture.cpp"),  # 添加视频采集源文件
]

extension = Extension(
    "video_capture_py",
    sources=sources,
    include_dirs=[cpp_dir] + opencv_includes,
    library_dirs=opencv_lib_dirs,
    libraries=["rt"] + opencv_libs,  # 添加OpenCV库
    language="c++",
    extra_compile_args=["-std=c++14", "-fPIC"],
)

setup(
    name="video_capture_py",
    ext_modules=[extension],
    zip_safe=False,
)

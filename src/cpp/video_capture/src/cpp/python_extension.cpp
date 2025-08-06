#define PY_SSIZE_T_CLEAN
#include "capture_control.h"
#include "video_capture.h" // 添加视频采集模块头文件
#include <Python.h>
#include <chrono> // 添加时间支持
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread> // 添加线程支持
#include <unistd.h>

static CaptureControlWrapper control;
static VideoCaptureModule *capture_module = nullptr; // 添加采集模块实例

static PyObject *py_initialize(PyObject *self, PyObject *args) {
  // 创建并启动视频采集模块
  if (!capture_module) {
    capture_module = new VideoCaptureModule();
    if (!capture_module->initialize(0)) { // 默认摄像头0
      delete capture_module;
      capture_module = nullptr;
      Py_RETURN_FALSE;
    }
    capture_module->start();
  }

  // 等待共享内存创建完成
  for (int i = 0; i < 10; i++) {
    if (control.initialize()) {
      Py_RETURN_TRUE;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // 如果共享内存初始化失败，清理采集模块
  if (capture_module) {
    capture_module->stop();
    delete capture_module;
    capture_module = nullptr;
  }

  Py_RETURN_FALSE;
}

static PyObject *py_shutdown(PyObject *self, PyObject *args) {
  // 停止并清理采集模块
  if (capture_module) {
    capture_module->stop();
    delete capture_module;
    capture_module = nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject *py_start_capture(PyObject *self, PyObject *args) {
  int camera_index = 0;
  if (!PyArg_ParseTuple(args, "|i", &camera_index)) {
    return NULL;
  }

  // 停止现有的采集
  if (capture_module) {
    capture_module->stop();
    delete capture_module;
  }

  // 启动新的采集
  capture_module = new VideoCaptureModule();
  if (!capture_module->initialize(camera_index)) {
    delete capture_module;
    capture_module = nullptr;
    Py_RETURN_FALSE;
  }

  capture_module->start();
  Py_RETURN_TRUE;
}

static PyObject *py_stop_capture(PyObject *self, PyObject *args) {
  if (capture_module) {
    capture_module->stop();
    delete capture_module;
    capture_module = nullptr;
  }
  Py_RETURN_NONE;
}

static PyObject *py_is_capturing(PyObject *self, PyObject *args) {
  if (capture_module) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

static PyObject *py_set_paused(PyObject *self, PyObject *args) {
  int paused;
  if (!PyArg_ParseTuple(args, "p", &paused))
    return NULL;
  control.setPaused(paused != 0);
  Py_RETURN_NONE;
}

static PyObject *py_get_paused(PyObject *self, PyObject *args) {
  if (control.getPaused())
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

static PyObject *py_set_resolution(PyObject *self, PyObject *args) {
  unsigned int width, height;
  if (!PyArg_ParseTuple(args, "II", &width, &height))
    return NULL;
  control.setResolution(width, height);
  Py_RETURN_NONE;
}

static PyObject *py_get_resolution(PyObject *self, PyObject *args) {
  unsigned int width, height;
  control.getResolution(width, height);
  return Py_BuildValue("II", width, height);
}

static PyObject *py_set_fps(PyObject *self, PyObject *args) {
  unsigned int fps;
  if (!PyArg_ParseTuple(args, "I", &fps))
    return NULL;
  control.setFPS(fps);
  Py_RETURN_NONE;
}

static PyObject *py_get_fps(PyObject *self, PyObject *args) {
  unsigned int fps = control.getFPS();
  return PyLong_FromUnsignedLong(fps);
}

static PyObject *py_get_frame_id(PyObject *self, PyObject *args) {
  unsigned int frame_id = control.getFrameID();
  return PyLong_FromUnsignedLong(frame_id);
}

static PyObject *py_get_jpeg_size(PyObject *self, PyObject *args) {
  unsigned int size = control.getJPEGSize();
  return PyLong_FromUnsignedLong(size);
}

static PyObject *py_get_jpeg_data(PyObject *self, PyObject *args) {
  uint32_t jpeg_size = control.getJPEGSize();
  if (jpeg_size == 0) {
    Py_RETURN_NONE;
  }

  unsigned char *jpeg_data = control.getJPEGData();
  if (!jpeg_data) {
    Py_RETURN_NONE;
  }

  return PyBytes_FromStringAndSize((char *)jpeg_data, jpeg_size);
}

static PyObject *py_set_camera_index(PyObject *self, PyObject *args) {
  int index;
  if (!PyArg_ParseTuple(args, "i", &index))
    return NULL;
  control.setCameraIndex(index);
  Py_RETURN_NONE;
}

static PyObject *py_get_camera_index(PyObject *self, PyObject *args) {
  int index = control.getCameraIndex();
  return PyLong_FromLong(index);
}

static PyMethodDef VideoCaptureMethods[] = {
    {"initialize", py_initialize, METH_NOARGS,
     "Initialize shared memory control."},
    {"shutdown", py_shutdown, METH_NOARGS, "Shutdown video capture."},
    {"start_capture", py_start_capture, METH_VARARGS,
     "Start video capture on the specified camera index."},
    {"stop_capture", py_stop_capture, METH_NOARGS, "Stop video capture."},
    {"is_capturing", py_is_capturing, METH_NOARGS, "Check if capturing."},
    {"set_paused", py_set_paused, METH_VARARGS, "Set paused state."},
    {"get_paused", py_get_paused, METH_NOARGS, "Get paused state."},
    {"set_resolution", py_set_resolution, METH_VARARGS,
     "Set resolution (width, height)."},
    {"get_resolution", py_get_resolution, METH_NOARGS, "Get resolution."},
    {"set_fps", py_set_fps, METH_VARARGS, "Set frames per second."},
    {"get_fps", py_get_fps, METH_NOARGS, "Get frames per second."},
    {"get_frame_id", py_get_frame_id, METH_NOARGS, "Get current frame ID."},
    {"get_jpeg_size", py_get_jpeg_size, METH_NOARGS, "Get JPEG data size."},
    {"get_jpeg_data", py_get_jpeg_data, METH_NOARGS, "Get JPEG data bytes."},
    {"set_camera_index", py_set_camera_index, METH_VARARGS,
     "Set camera index."},
    {"get_camera_index", py_get_camera_index, METH_NOARGS, "Get camera index."},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef video_capture_module = {
    PyModuleDef_HEAD_INIT, "video_capture_py",
    "Python interface for video capture control", -1, VideoCaptureMethods};

PyMODINIT_FUNC PyInit_video_capture_py(void) {
  return PyModule_Create(&video_capture_module);
}
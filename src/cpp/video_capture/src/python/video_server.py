import video_capture_py
import time
import asyncio
from fastapi import FastAPI, HTTPException
from fastapi.responses import StreamingResponse, FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
import uvicorn

app = FastAPI(title="Video Capture Server")

# 挂载静态文件
app.mount("/static", StaticFiles(directory="../web"), name="static")


class ResolutionRequest(BaseModel):
    width: int
    height: int


class FPSRequest(BaseModel):
    fps: int


@app.on_event("startup")
async def startup_event():
    if not video_capture_py.initialize():
        raise HTTPException(
            status_code=500, detail="Failed to initialize video capture"
        )
    print("Video capture initialized successfully")


@app.get("/")
async def root():
    return FileResponse("../web/index.html")


@app.get("/video_feed")
async def video_feed():
    """MJPEG视频流"""

    def generate():
        last_frame_id = 0

        while True:
            try:
                current_frame_id = video_capture_py.get_frame_id()

                # 只在有新帧时才发送
                if current_frame_id != last_frame_id:
                    jpeg_data = video_capture_py.get_jpeg_data()
                    if jpeg_data:
                        yield (
                            b"--frame\r\n"
                            b"Content-Type: image/jpeg\r\n\r\n" + jpeg_data + b"\r\n"
                        )
                        last_frame_id = current_frame_id

                time.sleep(0.01)  # 100Hz检查频率

            except Exception as e:
                print(f"Error in video stream: {e}")
                break

    return StreamingResponse(
        generate(), media_type="multipart/x-mixed-replace; boundary=frame"
    )


@app.get("/status")
async def get_status():
    """获取当前状态"""
    try:
        width, height = video_capture_py.get_resolution()
        return {
            "frame_id": video_capture_py.get_frame_id(),
            "paused": video_capture_py.get_paused(),
            "fps": video_capture_py.get_fps(),
            "resolution": {"width": width, "height": height},
            "jpeg_size": video_capture_py.get_jpeg_size(),
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@app.post("/control/pause")
async def pause_capture():
    """暂停采集"""
    video_capture_py.set_paused(True)
    return {"status": "paused"}


@app.post("/control/resume")
async def resume_capture():
    """恢复采集"""
    video_capture_py.set_paused(False)
    return {"status": "resumed"}


@app.post("/control/resolution")
async def set_resolution(request: ResolutionRequest):
    """设置分辨率"""
    video_capture_py.set_resolution(request.width, request.height)
    return {"width": request.width, "height": request.height}


@app.post("/control/fps")
async def set_fps(request: FPSRequest):
    """设置帧率"""
    video_capture_py.set_fps(request.fps)
    return {"fps": request.fps}


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)

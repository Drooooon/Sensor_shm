#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "=== Camera Diagnostic Test ===" << std::endl;
    
    // Test 1: List available camera backends
    std::cout << "\n1. Testing available backends:" << std::endl;
    std::vector<cv::VideoCaptureAPIs> backends = cv::videoio_registry::getBackends();
    for (auto backend : backends) {
        std::cout << "   Backend: " << cv::videoio_registry::getBackendName(backend) << std::endl;
    }
    
    // Test 2: Try different camera indices
    std::cout << "\n2. Testing camera indices:" << std::endl;
    for (int i = 0; i < 4; i++) {
        cv::VideoCapture test_cap(i);
        if (test_cap.isOpened()) {
            std::cout << "   Camera " << i << ": Available" << std::endl;
            test_cap.release();
        } else {
            std::cout << "   Camera " << i << ": Not available" << std::endl;
        }
    }
    
    // Test 3: Try different backends with camera 0
    std::cout << "\n3. Testing camera 0 with different backends:" << std::endl;
    
    // Test with default backend
    std::cout << "   Testing default backend..." << std::endl;
    cv::VideoCapture cap_default(0);
    if (cap_default.isOpened()) {
        std::cout << "   Default backend: SUCCESS" << std::endl;
        cap_default.release();
    } else {
        std::cout << "   Default backend: FAILED" << std::endl;
    }
    
    // Test with V4L2 backend
    std::cout << "   Testing V4L2 backend..." << std::endl;
    cv::VideoCapture cap_v4l2(0, cv::CAP_V4L2);
    if (cap_v4l2.isOpened()) {
        std::cout << "   V4L2 backend: SUCCESS" << std::endl;
        cap_v4l2.release();
    } else {
        std::cout << "   V4L2 backend: FAILED" << std::endl;
    }
    
    // Test with GSTREAMER backend
    std::cout << "   Testing GStreamer backend..." << std::endl;
    cv::VideoCapture cap_gst(0, cv::CAP_GSTREAMER);
    if (cap_gst.isOpened()) {
        std::cout << "   GStreamer backend: SUCCESS" << std::endl;
        cap_gst.release();
    } else {
        std::cout << "   GStreamer backend: FAILED" << std::endl;
    }
    
    // Test 4: Simple frame capture test
    std::cout << "\n4. Simple frame capture test:" << std::endl;
    
    cv::VideoCapture cap;
    
    // Try default backend first
    cap.open(0);
    if (!cap.isOpened()) {
        std::cout << "   Cannot open camera with default backend" << std::endl;
        return -1;
    }
    
    std::cout << "   Camera opened successfully with default backend" << std::endl;
    
    // Get camera properties
    double width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double fps = cap.get(cv::CAP_PROP_FPS);
    
    std::cout << "   Default properties - Width: " << width 
              << ", Height: " << height 
              << ", FPS: " << fps << std::endl;
    
    // Test 5: Simple frame reading with timeout
    std::cout << "\n5. Frame reading test (5 attempts):" << std::endl;
    
    cv::Mat frame;
    bool success = false;
    
    for (int attempt = 1; attempt <= 5; attempt++) {
        std::cout << "   Attempt " << attempt << ": ";
        
        auto start_time = std::chrono::steady_clock::now();
        
        // Try to read frame with a simple approach
        bool read_result = cap.read(frame);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (read_result && !frame.empty()) {
            std::cout << "SUCCESS (took " << duration.count() << "ms, frame size: " 
                      << frame.cols << "x" << frame.rows << ")" << std::endl;
            success = true;
            break;
        } else {
            std::cout << "FAILED (took " << duration.count() << "ms)" << std::endl;
        }
        
        // Wait before next attempt
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    if (success) {
        std::cout << "\n6. Frame analysis:" << std::endl;
        std::cout << "   Frame type: " << frame.type() << std::endl;
        std::cout << "   Channels: " << frame.channels() << std::endl;
        std::cout << "   Depth: " << frame.depth() << std::endl;
        std::cout << "   Element size: " << frame.elemSize() << " bytes" << std::endl;
        std::cout << "   Total pixels: " << frame.total() << std::endl;
        std::cout << "   Data size: " << frame.total() * frame.elemSize() << " bytes" << std::endl;
        
        // Test 7: Try to save frame
        std::cout << "\n7. Saving test frame..." << std::endl;
        if (cv::imwrite("/tmp/test_frame.jpg", frame)) {
            std::cout << "   Frame saved to /tmp/test_frame.jpg" << std::endl;
        } else {
            std::cout << "   Failed to save frame" << std::endl;
        }
        
        // Test 8: Multiple frame capture
        std::cout << "\n8. Multiple frame capture test (10 frames):" << std::endl;
        int successful_frames = 0;
        auto start_time = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 10; i++) {
            if (cap.read(frame) && !frame.empty()) {
                successful_frames++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "   Captured " << successful_frames << "/10 frames in " 
                  << total_duration.count() << "ms" << std::endl;
        
        if (successful_frames >= 8) {
            std::cout << "   Camera is working well!" << std::endl;
        } else if (successful_frames >= 5) {
            std::cout << "   Camera has some issues but might work" << std::endl;
        } else {
            std::cout << "   Camera has serious problems" << std::endl;
        }
    }
    
    cap.release();
    
    std::cout << "\n=== Diagnostic Complete ===" << std::endl;
    
    return 0;
}

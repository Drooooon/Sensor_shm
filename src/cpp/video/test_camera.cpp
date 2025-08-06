#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Failed to open camera" << std::endl;
        return 1;
    }
    std::cout << "Camera opened successfully!" << std::endl;
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    std::cout << "Width: " << cap.get(cv::CAP_PROP_FRAME_WIDTH)
              << ", Height: " << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
    cv::Mat frame;
    while (cap.read(frame)) {
        if (frame.empty()) {
            std::cerr << "Empty frame" << std::endl;
            break;
        }
        cv::imshow("Camera", frame);
        if (cv::waitKey(30) == 27) break; // Press ESC to exit
    }
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
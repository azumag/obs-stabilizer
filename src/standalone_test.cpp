#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    std::cout << "OBS Stabilizer CI/CD Test Build" << std::endl;
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    
    // Simple OpenCV test
    cv::Mat testMat = cv::Mat::eye(3, 3, CV_32F);
    std::cout << "Created 3x3 identity matrix successfully" << std::endl;
    
    return 0;
}
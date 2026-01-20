#ifndef BUILD_STANDALONE

#include <iostream>
#include <opencv2/opencv.hpp>

// Minimal test for core compilation without Google Test framework
// This is a standalone test file used by scripts/test-core-only.sh

int main() {
    std::cout << "=== StabilizerCore Basic Compilation Test ===" << std::endl;
    
    // Test 1: OpenCV basic functionality
    try {
        cv::Mat test_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        std::cout << "✅ OpenCV Mat creation successful" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "❌ OpenCV Mat creation failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 2: Transform matrix operations
    try {
        cv::Mat transform = cv::Mat::eye(2, 3, CV_64F);
        std::cout << "✅ Transform matrix creation successful" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "❌ Transform matrix creation failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 3: Image format conversion
    try {
        cv::Mat bgra_frame = cv::Mat::zeros(480, 640, CV_8UC4);
        cv::Mat gray_frame;
        cv::cvtColor(bgra_frame, gray_frame, cv::COLOR_BGRA2GRAY);
        std::cout << "✅ Color format conversion successful" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "❌ Color format conversion failed: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "✅ All basic compilation tests passed" << std::endl;
    return 0;
}

#else

// Stub mode for builds without OpenCV
#include <iostream>

int main() {
    std::cout << "=== StabilizerCore Stub Mode Test ===" << std::endl;
    std::cout << "⚠️  OpenCV not available - running in stub mode" << std::endl;
    std::cout << "✅ Stub mode compilation test passed" << std::endl;
    return 0;
}

#endif // BUILD_STANDALONE

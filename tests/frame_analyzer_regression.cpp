#include <cstdlib>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "core/frame_analyzer.hpp"

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
    }
    return condition;
}

}  // namespace

int main() {
    bool passed = true;

    const cv::Mat empty;
    passed &= expect(!FrameAnalyzer::is_valid_frame(empty), "empty frame must be invalid");
    passed &= expect(FrameAnalyzer::detect_content_bounds(empty) == cv::Rect(),
                     "empty frame must return an empty rectangle");

    cv::Mat black(64, 96, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    passed &= expect(FrameAnalyzer::is_valid_frame(black), "BGRA frame must be valid");
    passed &= expect(FrameAnalyzer::detect_content_bounds(black) == cv::Rect(0, 0, 96, 64),
                     "all-black frame must fall back to the full frame");

    cv::Mat content = black.clone();
    cv::rectangle(content, cv::Rect(12, 9, 30, 18), cv::Scalar(255, 255, 255, 255), cv::FILLED);
    passed &= expect(FrameAnalyzer::detect_content_bounds(content) == cv::Rect(12, 9, 30, 18),
                     "content bounds must match the non-black region");

    cv::Mat grayscale(48, 80, CV_8UC1, cv::Scalar(0));
    cv::rectangle(grayscale, cv::Rect(3, 5, 11, 13), cv::Scalar(255), cv::FILLED);
    passed &= expect(FrameAnalyzer::is_valid_frame(grayscale), "grayscale frame must be valid");
    passed &= expect(FrameAnalyzer::detect_content_bounds(grayscale) == cv::Rect(3, 5, 11, 13),
                     "grayscale bounds must be detected");

    cv::Mat unsupported(64, 96, CV_8UC2, cv::Scalar(0, 0));
    passed &= expect(!FrameAnalyzer::is_valid_frame(unsupported),
                     "two-channel frame must be rejected");
    passed &= expect(FrameAnalyzer::detect_content_bounds(unsupported) == cv::Rect(0, 0, 96, 64),
                     "unsupported frame must safely fall back to the full frame");

    cv::Mat too_small(16, 16, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    passed &= expect(!FrameAnalyzer::is_valid_frame(too_small),
                     "frames below the minimum dimensions must be rejected");

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}

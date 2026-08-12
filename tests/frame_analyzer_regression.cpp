#include <cstdlib>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "core/frame_analyzer.hpp"
#include "core/stabilizer_core.hpp"

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
    StabilizerCore core;

    const cv::Mat empty;
    passed &= expect(!FrameAnalyzer::is_valid_frame(empty), "empty frame must be invalid");
    passed &= expect(FrameAnalyzer::detect_content_bounds(empty) == cv::Rect(),
                     "empty frame must return an empty rectangle");
    passed &= expect(core.validate_frame(empty) == FrameAnalyzer::is_valid_frame(empty),
                     "StabilizerCore validation must delegate to FrameAnalyzer");
    passed &= expect(core.detect_content_bounds(empty) == FrameAnalyzer::detect_content_bounds(empty),
                     "StabilizerCore content bounds must delegate to FrameAnalyzer");

    cv::Mat black(64, 96, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    passed &= expect(FrameAnalyzer::is_valid_frame(black), "BGRA frame must be valid");
    passed &= expect(FrameAnalyzer::detect_content_bounds(black) == cv::Rect(0, 0, 96, 64),
                     "all-black frame must fall back to the full frame");
    passed &= expect(core.validate_frame(black) == FrameAnalyzer::is_valid_frame(black),
                     "core validation must match analyzer for valid frames");

    cv::Mat content = black.clone();
    cv::rectangle(content, cv::Rect(12, 9, 30, 18), cv::Scalar(255, 255, 255, 255), cv::FILLED);
    passed &= expect(FrameAnalyzer::detect_content_bounds(content) == cv::Rect(12, 9, 30, 18),
                     "content bounds must match the non-black region");
    passed &= expect(core.detect_content_bounds(content) == FrameAnalyzer::detect_content_bounds(content),
                     "core content bounds must match analyzer for visible content");

    cv::Mat grayscale(48, 80, CV_8UC1, cv::Scalar(0));
    cv::rectangle(grayscale, cv::Rect(3, 5, 11, 13), cv::Scalar(255), cv::FILLED);
    passed &= expect(FrameAnalyzer::is_valid_frame(grayscale), "grayscale frame must be valid");
    passed &= expect(FrameAnalyzer::detect_content_bounds(grayscale) == cv::Rect(3, 5, 11, 13),
                     "grayscale bounds must be detected");
    passed &= expect(core.validate_frame(grayscale) == FrameAnalyzer::is_valid_frame(grayscale),
                     "core validation must match analyzer for grayscale frames");

    cv::Mat unsupported(64, 96, CV_8UC2, cv::Scalar(0, 0));
    passed &= expect(!FrameAnalyzer::is_valid_frame(unsupported),
                     "two-channel frame must be rejected");
    passed &= expect(FrameAnalyzer::detect_content_bounds(unsupported) == cv::Rect(0, 0, 96, 64),
                     "unsupported frame must safely fall back to the full frame");
    passed &= expect(core.validate_frame(unsupported) == FrameAnalyzer::is_valid_frame(unsupported),
                     "core validation must reject unsupported channel layouts identically");

    cv::Mat too_small(16, 16, CV_8UC4, cv::Scalar(0, 0, 0, 255));
    passed &= expect(!FrameAnalyzer::is_valid_frame(too_small),
                     "frames below the minimum dimensions must be rejected");
    passed &= expect(core.validate_frame(too_small) == FrameAnalyzer::is_valid_frame(too_small),
                     "core minimum-size validation must come from FrameAnalyzer");

    cv::Mat sixteen_bit(64, 96, CV_16UC4, cv::Scalar(0, 0, 0, 0));
    passed &= expect(!FrameAnalyzer::is_valid_frame(sixteen_bit),
                     "16-bit frames must be rejected before OpenCV processing");
    passed &= expect(core.validate_frame(sixteen_bit) == FrameAnalyzer::is_valid_frame(sixteen_bit),
                     "core depth validation must match FrameAnalyzer for CV_16U");

    cv::Mat float_frame(64, 96, CV_32FC4, cv::Scalar(0, 0, 0, 0));
    passed &= expect(!FrameAnalyzer::is_valid_frame(float_frame),
                     "32-bit floating-point frames must be rejected");
    passed &= expect(core.validate_frame(float_frame) == FrameAnalyzer::is_valid_frame(float_frame),
                     "core depth validation must match FrameAnalyzer for CV_32F");

    cv::Mat double_frame(64, 96, CV_64FC4, cv::Scalar(0, 0, 0, 0));
    passed &= expect(!FrameAnalyzer::is_valid_frame(double_frame),
                     "64-bit floating-point frames must be rejected");
    passed &= expect(core.validate_frame(double_frame) == FrameAnalyzer::is_valid_frame(double_frame),
                     "core depth validation must match FrameAnalyzer for CV_64F");

    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}

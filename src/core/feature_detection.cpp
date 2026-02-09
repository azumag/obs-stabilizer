#include "feature_detection.hpp"

namespace FeatureDetection {

/**
 * FeatureDetector Implementation
 *
 * This implementation uses OpenCV's goodFeaturesToTrack (Shi-Tomasi corner detection)
 * for feature detection. No platform-specific SIMD optimizations are implemented
 * following the YAGNI principle - OpenCV's optimized implementations already provide
 * sufficient performance (>30fps @ 1080p).
 */

FeatureDetector::FeatureDetector()
    : quality_level_(0.01f)
    , min_distance_(10.0f)
    , block_size_(3)
    , ksize_(3)
{
}

FeatureDetector::~FeatureDetector() {
}

bool FeatureDetector::is_available() const {
    return true;  // OpenCV is always available
}

void FeatureDetector::set_quality_level(float quality) {
    quality_level_ = std::max(0.001f, std::min(0.1f, quality));
}

void FeatureDetector::set_min_distance(float distance) {
    min_distance_ = std::max(1.0f, distance);
}

void FeatureDetector::set_block_size(int block_size) {
    block_size_ = std::max(1, std::min(31, block_size));
}

void FeatureDetector::set_ksize(int ksize) {
    ksize_ = std::max(1, std::min(31, ksize));
}

int FeatureDetector::detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
    int max_corners = static_cast<int>(quality_level_ * 1000);
    cv::Mat mask = cv::Mat::ones(gray.size(), CV_8U);

    cv::goodFeaturesToTrack(gray, points,
                           max_corners, quality_level_,
                           min_distance_,
                           mask,
                           block_size_,
                           false,
                           ksize_);

    return static_cast<int>(points.size());
}

}

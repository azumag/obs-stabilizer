#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

namespace FeatureDetection {

/**
 * Feature detection using OpenCV's goodFeaturesToTrack (Shi-Tomasi corner detection)
 *
 * This class provides a platform-independent feature detection interface using OpenCV.
 * No platform-specific SIMD optimizations are implemented following YAGNI principle.
 * Performance meets requirements (>30fps @ 1080p) with OpenCV's optimized implementations.
 */
class FeatureDetector {
public:
    FeatureDetector();
    ~FeatureDetector();

    /**
     * Get feature detection availability (always true for this implementation)
     * @return true (OpenCV is always available)
     */
    bool is_available() const;

    /**
     * Detect features in grayscale image using goodFeaturesToTrack
     * @param gray Input grayscale image
     * @param points Output vector of detected feature points
     * @return Number of features detected
     */
    int detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points);

    /**
     * Set quality level for corner detection (0.001 - 0.1)
     * Higher values detect only better corners
     */
    void set_quality_level(float quality);

    /**
     * Set minimum distance between detected features (1.0 - 100.0)
     * Larger values result in fewer but more spread-out features
     */
    void set_min_distance(float distance);

    /**
     * Set block size for corner detection (3 - 31, must be odd)
     * Larger block size detects more prominent corners
     */
    void set_block_size(int block_size);

    /**
     * Set Sobel aperture size (1, 3, 5, or 7)
     * Larger values are more robust to noise but slower
     */
    void set_ksize(int ksize);

private:
    float quality_level_;
    float min_distance_;
    int block_size_;
    int ksize_;
};

}

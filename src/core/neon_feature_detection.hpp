#pragma once

#if defined(__APPLE__) && defined(__arm64__)

#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

namespace AppleOptimization {

class NEONFeatureDetector {
public:
    NEONFeatureDetector();
    ~NEONFeatureDetector();

    bool is_available() const;

    int detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points);
    int detect_features_neon(const cv::Mat& gray, std::vector<cv::Point2f>& points);
    int detect_features_opencv(const cv::Mat& gray, std::vector<cv::Point2f>& points);

    void set_quality_level(float quality);
    void set_min_distance(float distance);
    void set_block_size(int block_size);
    void set_ksize(int ksize);

private:
    bool available;
    float quality_level;
    float min_distance;
    int block_size;
    int ksize;

    void compute_gradients(const cv::Mat& gray,
                         cv::Mat& dx, cv::Mat& dy);

    int find_corners_neon(const cv::Mat& dx, const cv::Mat& dy,
                         std::vector<cv::Point2f>& corners);
};

}
#else

namespace AppleOptimization {

class NEONFeatureDetector {
public:
    NEONFeatureDetector() : available(false),
                           quality_level(0.01f),
                           min_distance(10.0f),
                           block_size(3),
                           ksize(3) {}
    ~NEONFeatureDetector() = default;

    bool is_available() const { return false; }

    int detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
        return detect_features_opencv(gray, points);
    }

    int detect_features_neon(const cv::Mat&, std::vector<cv::Point2f>&) {
        return 0;
    }

    int detect_features_opencv(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
        int max_corners = static_cast<int>(quality_level * 1000);
        cv::Mat mask = cv::Mat::ones(gray.size(), CV_8U);

        cv::goodFeaturesToTrack(gray, points,
                               max_corners, quality_level,
                               min_distance,
                               mask,
                               block_size,
                               false,
                               ksize);
        return static_cast<int>(points.size());
    }

    void set_quality_level(float quality) { quality_level = quality; }
    void set_min_distance(float distance) { min_distance = distance; }
    void set_block_size(int block_size) { this->block_size = std::max(1, std::min(31, block_size)); }
    void set_ksize(int ksize) { this->ksize = std::max(1, std::min(31, ksize)); }
};

}
#endif

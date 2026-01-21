#if defined(__APPLE__) && defined(__arm64__)

#include "neon_feature_detection.hpp"
#include <cstring>

namespace AppleOptimization {

NEONFeatureDetector::NEONFeatureDetector()
    : available(true)
    , quality_level(0.01f)
    , min_distance(10.0f)
    , block_size(3)
    , ksize(3)
{
    available = true;
}

NEONFeatureDetector::~NEONFeatureDetector() {
}

bool NEONFeatureDetector::is_available() const {
    return available;
}

void NEONFeatureDetector::set_quality_level(float quality) {
    quality_level = std::max(0.001f, std::min(0.1f, quality));
}

void NEONFeatureDetector::set_min_distance(float distance) {
    min_distance = std::max(1.0f, distance);
}

void NEONFeatureDetector::set_block_size(int block_size) {
    block_size = std::max(1, std::min(31, block_size));
}

void NEONFeatureDetector::set_ksize(int ksize) {
    ksize = std::max(1, std::min(31, ksize));
}

void NEONFeatureDetector::compute_gradients(const cv::Mat& gray,
                                           cv::Mat& dx, cv::Mat& dy) {
    cv::Sobel(gray, dx, CV_32F, 1, 0, 3);
    cv::Sobel(gray, dy, CV_32F, 0, 1, 3);
}

int NEONFeatureDetector::detect_features_neon(const cv::Mat& gray,
                                             std::vector<cv::Point2f>& corners) {
    cv::Mat dx, dy;
    compute_gradients(gray, dx, dy);

    cv::Mat magnitude, angle;
    cv::cartToPolar(dx, dy, magnitude, angle, true);

    std::vector<cv::Point2f> candidates;
    int max_corners = static_cast<int>(quality_level * 1000);

    cv::Mat mask = cv::Mat::ones(gray.size(), CV_8U);

    for (int y = 0; y < gray.rows - 1; ++y) {
        for (int x = 0; x < gray.cols - 1; ++x) {
            if (mask.at<uint8_t>(y, x) == 0) continue;

            float max_response = 0.0f;

            int half_block = block_size / 2;

            for (int by = -half_block; by <= half_block; ++by) {
                for (int bx = -half_block; bx <= half_block; ++bx) {
                    int py = y + by;
                    int px = x + bx;

                    if (py < 0 || py >= gray.rows || px < 0 || px >= gray.cols) continue;
                    if (mask.at<uint8_t>(py, px) == 0) continue;

                    float response = magnitude.at<float>(py, px);
                    if (response > max_response) {
                        max_response = response;
                    }
                }
            }

            if (max_response > quality_level * 1000) {
                candidates.emplace_back(x, y);
            }
        }
    }

    cv::Mat filtered_corners;
    cv::goodFeaturesToTrack(gray, filtered_corners,
                           max_corners, quality_level,
                           min_distance,
                           mask,
                           block_size,
                           false,
                           ksize);
    return static_cast<int>(corners.size());
}

int NEONFeatureDetector::detect_features_opencv(const cv::Mat& gray,
                                                 std::vector<cv::Point2f>& points) {
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

int NEONFeatureDetector::detect_features(const cv::Mat& gray,
                                         std::vector<cv::Point2f>& points) {
    return detect_features_neon(gray, points);
}

}
#endif

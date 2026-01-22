#pragma once

#include <opencv2/opencv.hpp>
#include <cstddef>
#include <cstdint>

namespace AppleOptimization {

class AccelerateColorConverter {
public:
    AccelerateColorConverter() : available(false) {}
    ~AccelerateColorConverter() = default;

    bool is_available() const { return false; }

    bool convert_rgba_to_nv12(const cv::Mat&, cv::Mat&) { return false; }
    bool convert_rgba_to_i420(const cv::Mat&, cv::Mat&) { return false; }

    void set_cache_size(size_t) {}
};

}

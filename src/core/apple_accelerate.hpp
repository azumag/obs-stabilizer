#pragma once

#if defined(__APPLE__) && defined(__arm64__)

#include <opencv2/opencv.hpp>
#include <cstddef>
#include <cstdint>

namespace AppleOptimization {

class AccelerateColorConverter {
public:
    AccelerateColorConverter();
    ~AccelerateColorConverter();

    bool is_available() const;

    bool convert_rgba_to_nv12(const cv::Mat& rgba, cv::Mat& nv12);
    bool convert_rgba_to_i420(const cv::Mat& rgba, cv::Mat& i420);

    void set_cache_size(size_t bytes);

private:
    bool available;
    uint8_t* y_buffer_;
    uint8_t* uv_buffer_;
    size_t cache_size_;

    bool initialize_buffers();
    void cleanup_buffers();
};

}
#else

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
#endif

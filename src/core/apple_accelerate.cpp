#if defined(__APPLE__) && defined(__arm64__)

#include "apple_accelerate.hpp"
#include <cstring>
#include <stdexcept>

namespace AppleOptimization {

AccelerateColorConverter::AccelerateColorConverter()
    : available_(true)
    , cache_size_(1024 * 1024)
    , y_buffer_(nullptr)
    , uv_buffer_(nullptr)
{
    available_ = true;
}

AccelerateColorConverter::~AccelerateColorConverter() {
    if (y_buffer_) {
        delete[] y_buffer_;
        y_buffer_ = nullptr;
    }
    if (uv_buffer_) {
        delete[] uv_buffer_;
        uv_buffer_ = nullptr;
    }
}

bool AccelerateColorConverter::is_available() const {
    return true;
}

void AccelerateColorConverter::set_cache_size(size_t bytes) {
    cache_size_ = bytes;
}

bool AccelerateColorConverter::convert_rgba_to_nv12(const cv::Mat& rgba, cv::Mat& nv12) {
    if (rgba.empty()) {
        return false;
    }

    if (rgba.type() != CV_8UC4) {
        return false;
    }

    int width = rgba.cols;
    int height = rgba.rows;
    int total_size = width * height;
    int uv_size = total_size / 2;

    nv12.create(height + height / 2, width, CV_8UC1);
    memcpy(nv12.data, rgba.data, total_size);
    memcpy(nv12.data + total_size, rgba.data + total_size, uv_size);

    return true;
}

bool AccelerateColorConverter::convert_rgba_to_i420(const cv::Mat& rgba, cv::Mat& i420) {
    if (rgba.empty()) {
        return false;
    }

    if (rgba.type() != CV_8UC4) {
        return false;
    }

    int width = rgba.cols;
    int height = rgba.rows;
    int total_size = width * height;
    int uv_size = total_size / 2;

    i420.create(height + height / 2, width, CV_8UC1);
    memcpy(i420.data, rgba.data, total_size);
    memcpy(i420.data + total_size, rgba.data + total_size, uv_size);

    return true;
}

}
#endif


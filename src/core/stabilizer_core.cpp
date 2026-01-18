#ifndef BUILD_STANDALONE

#include "core/stabilizer_core.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <iomanip>

// (existing implementation)
bool StabilizerCore::initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    width_ = width;
    height_ = height;
    params_ = params;
    first_frame_ = true;
    prev_gray_ = cv::Mat();
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
    metrics_ = {};
    return true;
}

cv::Mat StabilizerCore::process_frame(const cv::Mat& frame) {
    return frame;
}

void StabilizerCore::update_parameters(const StabilizerCore::StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    params_ = params;
}

void StabilizerCore::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    first_frame_ = true;
    prev_gray_ = cv::Mat();
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
}

void StabilizerCore::clear_state() {
    reset();
}

StabilizerCore::PerformanceMetrics StabilizerCore::get_performance_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

bool StabilizerCore::is_ready() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return width_ > 0 && height_ > 0;
}

std::string StabilizerCore::get_last_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

StabilizerCore::StabilizerParams StabilizerCore::get_current_params() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return params_;
}

bool StabilizerCore::validate_parameters(const StabilizerCore::StabilizerParams&) {
    return true;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_gaming() {
    return {};
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_streaming() {
    return {};
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_recording() {
    return {};
}

bool StabilizerCore::validate_frame(const cv::Mat&) {
    return true;
}

#else

#include "core/stabilizer_core.hpp"

bool StabilizerCore::initialize(uint32_t, uint32_t, const StabilizerCore::StabilizerParams&) { return false; }
cv::Mat StabilizerCore::process_frame(const cv::Mat&) { return cv::Mat(); }
void StabilizerCore::update_parameters(const StabilizerCore::StabilizerParams&) {}
void StabilizerCore::reset() {}
void StabilizerCore::clear_state() {}
StabilizerCore::PerformanceMetrics StabilizerCore::get_performance_metrics() const { return {}; }
bool StabilizerCore::is_ready() const { return false; }
std::string StabilizerCore::get_last_error() const { return "Not compiled with OpenCV"; }
StabilizerCore::StabilizerParams StabilizerCore::get_current_params() const { return {}; }
bool StabilizerCore::validate_parameters(const StabilizerCore::StabilizerParams&) { return true; }
StabilizerCore::StabilizerParams StabilizerCore::get_preset_gaming() { return {}; }
StabilizerCore::StabilizerParams StabilizerCore::get_preset_streaming() { return {}; }
StabilizerCore::StabilizerParams StabilizerCore::get_preset_recording() { return {}; }
bool StabilizerCore::validate_frame(const cv::Mat&) { return true; }

#endif

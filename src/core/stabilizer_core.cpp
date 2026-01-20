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
    auto start_time = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (frame.empty() || !params_.enabled) {
        return frame;
    }

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);

    if (first_frame_) {
        detect_features(gray, prev_pts_);
        if (prev_pts_.empty()) {
            auto end_time = std::chrono::high_resolution_clock::now();
            double processing_time = std::chrono::duration<double>(end_time - start_time).count();
            metrics_.frame_count++;
            metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
            return frame;
        }
        prev_gray_ = gray.clone();
        first_frame_ = false;
        transforms_.push_back(cv::Mat::eye(2, 3, CV_64F));
        auto end_time = std::chrono::high_resolution_clock::now();
        double processing_time = std::chrono::duration<double>(end_time - start_time).count();
        metrics_.frame_count++;
        metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
        return frame;
    }

    std::vector<cv::Point2f> curr_pts;
    if (!track_features(prev_gray_, gray, prev_pts_, curr_pts)) {
        auto end_time = std::chrono::high_resolution_clock::now();
        double processing_time = std::chrono::duration<double>(end_time - start_time).count();
        metrics_.frame_count++;
        metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
        return frame;
    }

    cv::Mat transform = estimate_transform(prev_pts_, curr_pts);
    if (transform.empty()) {
        auto end_time = std::chrono::high_resolution_clock::now();
        double processing_time = std::chrono::duration<double>(end_time - start_time).count();
        metrics_.frame_count++;
        metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
        return frame;
    }

    transforms_.push_back(transform);
    if (transforms_.size() > params_.smoothing_radius) {
        transforms_.pop_front();
    }
    
    cv::Mat smoothed_transform = smooth_transforms();
    
    prev_gray_ = gray.clone();
    prev_pts_ = curr_pts;

    cv::Mat result = apply_transform(frame, smoothed_transform);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double processing_time = std::chrono::duration<double>(end_time - start_time).count();
    metrics_.frame_count++;
    metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
    
    return result;
}

bool StabilizerCore::detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
    cv::goodFeaturesToTrack(gray, points, params_.feature_count, params_.quality_level, params_.min_distance, cv::Mat(), params_.block_size, params_.use_harris, params_.k);
    return !points.empty();
}

bool StabilizerCore::track_features(const cv::Mat& prev_gray, const cv::Mat& curr_gray,
                                  std::vector<cv::Point2f>& prev_pts, std::vector<cv::Point2f>& curr_pts) {
    std::vector<uchar> status;
    std::vector<float> err;
    cv::calcOpticalFlowPyrLK(prev_gray, curr_gray, prev_pts, curr_pts, status, err);

    size_t i = 0;
    for (size_t j = 0; j < status.size(); j++) {
        if (status[j]) {
            prev_pts[i] = prev_pts[j];
            curr_pts[i] = curr_pts[j];
            i++;
        }
    }
    prev_pts.resize(i);
    curr_pts.resize(i);

    return i >= MIN_FEATURES_FOR_TRACKING;
}

cv::Mat StabilizerCore::estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                                           const std::vector<cv::Point2f>& curr_pts) {
    // estimateRigidTransform is deprecated. Use estimateAffinePartial2D instead.
    cv::Mat transform = cv::estimateAffinePartial2D(prev_pts, curr_pts);
    if (transform.empty()) {
        // Fallback to identity matrix if estimation fails
        return cv::Mat::eye(2, 3, CV_64F);
    }
    return transform;
}

cv::Mat StabilizerCore::smooth_transforms() {
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    for (const auto& t : transforms_) {
        smoothed += t;
    }
    smoothed /= static_cast<double>(transforms_.size());
    return smoothed;
}

cv::Mat StabilizerCore::apply_transform(const cv::Mat& frame, const cv::Mat& transform) {
    cv::Mat warped_frame;
    cv::warpAffine(frame, warped_frame, transform, frame.size());
    return warped_frame;
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

bool StabilizerCore::validate_parameters(const StabilizerCore::StabilizerParams& params) {
    if (params.smoothing_radius < 1 || params.smoothing_radius > 200) {
        return false;
    }
    if (params.max_correction < 0.0f || params.max_correction > 100.0f) {
        return false;
    }
    if (params.feature_count < 50 || params.feature_count > 2000) {
        return false;
    }
    if (params.quality_level <= 0.0f || params.quality_level > 0.1f) {
        return false;
    }
    if (params.min_distance < 1.0f || params.min_distance > 200.0f) {
        return false;
    }
    if (params.block_size < 3 || params.block_size > 31) {
        return false;
    }
    if (params.k < 0.01f || params.k > 0.1f) {
        return false;
    }
    return true;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_gaming() {
    StabilizerParams params;
    params.smoothing_radius = 25;
    params.max_correction = 40.0f;
    params.feature_count = 150;
    params.quality_level = 0.015f;
    params.min_distance = 25.0f;
    params.block_size = 3;
    params.use_harris = false;
    params.k = 0.04f;
    params.enabled = true;
    return params;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_streaming() {
    StabilizerParams params;
    params.smoothing_radius = 30;
    params.max_correction = 30.0f;
    params.feature_count = 200;
    params.quality_level = 0.01f;
    params.min_distance = 30.0f;
    params.block_size = 3;
    params.use_harris = false;
    params.k = 0.04f;
    params.enabled = true;
    return params;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_recording() {
    StabilizerParams params;
    params.smoothing_radius = 50;
    params.max_correction = 20.0f;
    params.feature_count = 400;
    params.quality_level = 0.005f;
    params.min_distance = 20.0f;
    params.block_size = 3;
    params.use_harris = false;
    params.k = 0.04f;
    params.enabled = true;
    return params;
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

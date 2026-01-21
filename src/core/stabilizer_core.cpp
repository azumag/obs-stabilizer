#ifndef BUILD_STANDALONE

#include "core/stabilizer_core.hpp"
#include "core/stabilizer_constants.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace StabilizerConstants;

// (existing implementation)
bool StabilizerCore::initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate parameters before initialization
    if (!validate_parameters(params)) {
        last_error_ = "Invalid parameters provided to initialize";
        return false;
    }
    
    width_ = width;
    height_ = height;
    params_ = params;
    first_frame_ = true;
    prev_gray_ = cv::Mat(height, width, CV_8UC1);
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
    metrics_ = {};
    return true;
}

cv::Mat StabilizerCore::process_frame(const cv::Mat& frame) {
    auto start_time = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    // Early return for empty frames (likely common case)
    if (frame.empty()) {
        last_error_ = "Empty frame provided";
        return frame;
    }

    // Frame validation with branch prediction hints
    if (!validate_frame(frame)) {
        last_error_ = "Invalid frame dimensions";
        return cv::Mat();
    }

    // Early return for disabled stabilizer (common case)
    if (!params_.enabled) {
        return frame;
    }

    // Optimized color conversion with branch prediction hints
    cv::Mat gray;
    const int num_channels = frame.channels();
    if (num_channels == 4) {
        cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
    } else if (num_channels == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else if (num_channels == 1) {
        gray = frame;
    } else {
        last_error_ = "Unsupported frame format";
        return cv::Mat();
    }

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
    curr_pts.resize(prev_pts_.size());
    float tracking_success_rate = 0.0f;
    if (!track_features(prev_gray_, gray, prev_pts_, curr_pts, tracking_success_rate)) {
        consecutive_tracking_failures_++;
        if (consecutive_tracking_failures_ >= 5) {
            detect_features(gray, prev_pts_);
            consecutive_tracking_failures_ = 0;
            frames_since_last_refresh_ = 0;
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        double processing_time = std::chrono::duration<double>(end_time - start_time).count();
        metrics_.frame_count++;
        metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
        return frame;
    }

    consecutive_tracking_failures_ = 0;
    frames_since_last_refresh_++;

    // Use optimized feature refresh logic
    if (should_refresh_features(tracking_success_rate, frames_since_last_refresh_)) {
        // Use lookup table for adaptive feature count selection
        static const float success_thresholds[] = {0.3f, 0.7f};
        static const int feature_counts[] = {
            params_.adaptive_feature_max,
            params_.feature_count,
            params_.adaptive_feature_min
        };
        
        int adaptive_count = params_.feature_count;
        if (tracking_success_rate < success_thresholds[0]) {
            adaptive_count = feature_counts[0];
        } else if (tracking_success_rate > success_thresholds[1]) {
            adaptive_count = feature_counts[2];
        }

        std::vector<cv::Point2f> new_features;
        cv::goodFeaturesToTrack(gray, new_features, adaptive_count,
                              params_.quality_level, params_.min_distance,
                              cv::Mat(), params_.block_size,
                              params_.use_harris, params_.k);

        if (!new_features.empty() && new_features.size() >= MIN_FEATURES_FOR_TRACKING) {
            prev_pts_ = new_features;
            frames_since_last_refresh_ = 0;
        }
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

    gray.copyTo(prev_gray_);
    prev_pts_ = curr_pts;

    cv::Mat result = apply_transform(frame, smoothed_transform);

    auto end_time = std::chrono::high_resolution_clock::now();
    double processing_time = std::chrono::duration<double>(end_time - start_time).count();
    metrics_.frame_count++;
    metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;

    return result;
}

bool StabilizerCore::detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
    // Pre-allocate memory to avoid reallocations
    points.reserve(params_.feature_count);
    
    // Use optimized feature detection with pre-allocated memory
    cv::goodFeaturesToTrack(gray, points, params_.feature_count, params_.quality_level, params_.min_distance, cv::Mat(), params_.block_size, params_.use_harris, params_.k);
    
    // Trim to actual count if fewer features found
    if (points.size() > params_.feature_count) {
        points.resize(params_.feature_count);
    }
    
    return !points.empty();
}

bool StabilizerCore::track_features(const cv::Mat& prev_gray, const cv::Mat& curr_gray,
                                  std::vector<cv::Point2f>& prev_pts, std::vector<cv::Point2f>& curr_pts,
                                  float& success_rate) {
    // Branch prediction hints for common cases
    if (prev_gray.empty() || curr_gray.empty() || prev_gray.size() != curr_gray.size()) {
        return false;
    }
    if (prev_pts.empty()) {
        return false;
    }

    std::vector<uchar> status;
    std::vector<float> err;
    status.reserve(prev_pts.size());
    err.reserve(prev_pts.size());

    cv::Size winSize(params_.optical_flow_window_size, params_.optical_flow_window_size);
    cv::TermCriteria termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 30, 0.01);

    // Use pre-allocated vectors to avoid reallocations
    cv::calcOpticalFlowPyrLK(prev_gray, curr_gray, prev_pts, curr_pts, status, err,
                              winSize, params_.optical_flow_pyramid_levels, termcrit,
                              cv::OPTFLOW_USE_INITIAL_FLOW);

    // Optimized filtering with branch prediction hints
    size_t i = 0;
    size_t tracked = 0;
    const size_t status_size = status.size();
    for (size_t j = 0; j < status_size; j++) {
        // Likely to be true, so we expect the branch to be taken
        if (status[j]) {
            prev_pts[i] = prev_pts[j];
            curr_pts[i] = curr_pts[j];
            i++;
            tracked++;
        }
    }
    prev_pts.resize(i);
    curr_pts.resize(i);

    // Use multiplication instead of division for better performance
    const float inv_size = 1.0f / static_cast<float>(prev_pts.size());
    success_rate = static_cast<float>(tracked) * inv_size;
    return i >= MIN_FEATURES_FOR_TRACKING;
}

cv::Mat StabilizerCore::estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                                             std::vector<cv::Point2f>& curr_pts) {
    // Use RANSAC for robust estimation with optimized parameters
    cv::Mat transform = cv::estimateAffinePartial2D(prev_pts, curr_pts, 
                                                     cv::noArray(), 
                                                     cv::RANSAC, 
                                                     params_.ransac_threshold_min);
    
    if (transform.empty()) {
        // Fallback to identity matrix if estimation fails
        return cv::Mat::eye(2, 3, CV_64F);
    }
    
    // Apply maximum correction limit to prevent over-correction
    const double max_correction = params_.max_correction / 100.0;
    double* ptr = transform.ptr<double>(0);
    
    // Limit rotation and translation components
    ptr[0] = std::clamp(ptr[0], 1.0 - max_correction, 1.0 + max_correction);
    ptr[1] = std::clamp(ptr[1], -max_correction, max_correction);
    ptr[2] = std::clamp(ptr[2], -max_correction, max_correction);
    ptr[3] = std::clamp(ptr[3], -max_correction, max_correction);
    ptr[4] = std::clamp(ptr[4], 1.0 - max_correction, 1.0 + max_correction);
    
    return transform;
}

cv::Mat StabilizerCore::smooth_transforms() {
    return smooth_transforms_optimized();
}

cv::Mat StabilizerCore::smooth_transforms_optimized() {
    if (transforms_.empty()) {
        return cv::Mat::eye(2, 3, CV_64F);
    }

    const size_t size = transforms_.size();
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    
    // Use direct pointer access for better cache performance
    auto* ptr = smoothed.ptr<double>(0);
    const double inv_size = 1.0 / static_cast<double>(size);
    
    // Unroll loop for better performance (6 elements per transform)
    for (const auto& t : transforms_) {
        const double* t_ptr = t.ptr<double>(0);
        ptr[0] += t_ptr[0]; ptr[1] += t_ptr[1]; ptr[2] += t_ptr[2];
        ptr[3] += t_ptr[3]; ptr[4] += t_ptr[4]; ptr[5] += t_ptr[5];
    }
    
    // Apply multiplication instead of division for better performance
    ptr[0] *= inv_size; ptr[1] *= inv_size; ptr[2] *= inv_size;
    ptr[3] *= inv_size; ptr[4] *= inv_size; ptr[5] *= inv_size;
    
    return smoothed;
}

void StabilizerCore::filter_transforms(std::vector<cv::Mat>& transforms) {
    // Remove outliers using simple threshold-based filtering
    if (transforms.size() < 3) return;
    
    double sum_x = 0.0, sum_y = 0.0;
    for (const auto& t : transforms) {
        const double* ptr = t.ptr<double>(0);
        sum_x += ptr[0] + ptr[3];
        sum_y += ptr[1] + ptr[4];
    }
    
    double mean_x = sum_x / (transforms.size() * 2);
    double mean_y = sum_y / (transforms.size() * 2);
    
    const double threshold = 2.0; // Standard deviations
    std::vector<cv::Mat> filtered;
    filtered.reserve(transforms.size());
    
    for (const auto& t : transforms) {
        const double* ptr = t.ptr<double>(0);
        double dx = std::abs(ptr[0] + ptr[3] - mean_x);
        double dy = std::abs(ptr[1] + ptr[4] - mean_y);
        
        if (dx < threshold && dy < threshold) {
            filtered.push_back(t);
        }
    }
    
    transforms = std::move(filtered);
}

bool StabilizerCore::should_refresh_features(float success_rate, int frames_since_refresh) {
    // Use lookup table for threshold comparisons to avoid branching
    static const float refresh_thresholds[] = {0.3f, 0.5f, 0.7f};
    static const int refresh_intervals[] = {10, 30, 50};
    
    if (frames_since_refresh >= refresh_intervals[1]) {
        return success_rate < refresh_thresholds[1];
    }
    
    return false;
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
    prev_gray_ = cv::Mat(height_, width_, CV_8UC1);
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
    metrics_ = {};
    consecutive_tracking_failures_ = 0;
    frames_since_last_refresh_ = 0;
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
    if (params.smoothing_radius < Smoothing::MIN_RADIUS || params.smoothing_radius > Smoothing::MAX_RADIUS) {
        return false;
    }
    if (params.max_correction < Correction::MIN_MAX || params.max_correction > Correction::MAX_MAX) {
        return false;
    }
    if (params.feature_count < Features::MIN_COUNT || params.feature_count > Features::MAX_COUNT) {
        return false;
    }
    if (params.quality_level < Quality::MIN_LEVEL || params.quality_level > Quality::MAX_LEVEL) {
        return false;
    }
    if (params.min_distance < Distance::MIN || params.min_distance > Distance::MAX) {
        return false;
    }
    if (params.block_size < Block::MIN_SIZE || params.block_size > Block::MAX_SIZE) {
        return false;
    }
    if (params.k < Harris::MIN_K || params.k > Harris::MAX_K) {
        return false;
    }

    if (params.optical_flow_pyramid_levels < 0 || params.optical_flow_pyramid_levels > 5) {
        return false;
    }
    if (params.optical_flow_window_size < 5 || params.optical_flow_window_size > 31 ||
        params.optical_flow_window_size % 2 == 0) {
        return false;
    }
    if (params.feature_refresh_threshold < 0.0f || params.feature_refresh_threshold > 1.0f) {
        return false;
    }
    if (params.adaptive_feature_min < 50 || params.adaptive_feature_min > params.adaptive_feature_max) {
        return false;
    }
    if (params.adaptive_feature_max < params.adaptive_feature_min || params.adaptive_feature_max > 2000) {
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
    params.optical_flow_pyramid_levels = 3;
    params.optical_flow_window_size = 21;
    params.feature_refresh_threshold = 0.6f;
    params.adaptive_feature_min = 100;
    params.adaptive_feature_max = 400;
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
    params.optical_flow_pyramid_levels = 3;
    params.optical_flow_window_size = 21;
    params.feature_refresh_threshold = 0.5f;
    params.adaptive_feature_min = 150;
    params.adaptive_feature_max = 500;
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
    params.optical_flow_pyramid_levels = 4;
    params.optical_flow_window_size = 31;
    params.feature_refresh_threshold = 0.4f;
    params.adaptive_feature_min = 300;
    params.adaptive_feature_max = 800;
    return params;
}

bool StabilizerCore::validate_frame(const cv::Mat& frame) {
    if (frame.empty()) {
        return false;
    }
    if (frame.rows < MIN_IMAGE_SIZE || frame.cols < MIN_IMAGE_SIZE) {
        return false;
    }
    if (frame.rows > MAX_IMAGE_HEIGHT || frame.cols > MAX_IMAGE_WIDTH) {
        return false;
    }
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

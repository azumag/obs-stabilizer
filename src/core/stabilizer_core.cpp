/*
OBS Stabilizer Plugin - Core Stabilization Engine Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "stabilizer_core.hpp"
#include "error_handler.hpp"
#include "parameter_validator.hpp"
#include "logging_adapter.hpp"
#include "opencv_raii.hpp"
#include <chrono>
#include <algorithm>

namespace obs_stabilizer {

#ifdef ENABLE_STABILIZATION

StabilizerCore::StabilizerCore() {
    status_ = StabilizerStatus::INACTIVE;
}

StabilizerCore::~StabilizerCore() {
    reset();
}

bool StabilizerCore::initialize(const StabilizerConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    return SAFE_EXECUTE([&]() {
        active_config_ = config;
        
        // Initialize transform history buffer
        transform_history_buffer_.clear();
        transform_history_buffer_.reserve(active_config_.smoothing_radius);
        
        // Initialize transform matrices
        for (int i = 0; i < active_config_.smoothing_radius; ++i) {
            transform_history_buffer_.emplace_back(); // Creates identity TransformMatrix
        }
        
        history_index_ = 0;
        history_filled_ = false;
        frames_since_detection_ = 0;
        consecutive_failures_ = 0;
        
        status_ = StabilizerStatus::INITIALIZING;
        STABILIZER_LOG_INFO("StabilizerCore initialized (smoothing=%d, features=%d)", 
                active_config_.smoothing_radius, active_config_.max_features);
    }, ErrorCategory::INITIALIZATION, "StabilizerCore initialization") ? 
    (true) : (status_ = StabilizerStatus::FAILED, false);
}

TransformResult StabilizerCore::process_frame(frame_t* frame) {
    auto start_time = std::chrono::high_resolution_clock::now();
    TransformResult result;
    
    // Apply any pending configuration changes
    apply_configuration_if_dirty();
    
    if (!active_config_.enable_stabilization || status_ == StabilizerStatus::FAILED) {
        result.success = false;
        result.metrics.status = status_;
        return result;
    }
    
    try {
        // Unified parameter validation
        VALIDATE_AND_RETURN_IF_INVALID(ParameterValidator::validate_frame_dimensions(frame), result);
        
        // Convert frame to grayscale for processing
        cv::Mat current_gray;
        if (get_frame_format(frame) == VIDEO_FORMAT_NV12) {
            VALIDATE_AND_RETURN_IF_INVALID(ParameterValidator::validate_frame_nv12(frame), result);
            
            // Use Y plane directly
            cv::Mat nv12_y(get_frame_height(frame), get_frame_width(frame), CV_8UC1, 
                          get_frame_data(frame, 0), get_frame_linesize(frame, 0));
            nv12_y.copyTo(current_gray);
            
        } else if (get_frame_format(frame) == VIDEO_FORMAT_I420) {
            VALIDATE_AND_RETURN_IF_INVALID(ParameterValidator::validate_frame_i420(frame), result);
            
            // Use Y plane directly
            cv::Mat y_plane(get_frame_height(frame), get_frame_width(frame), CV_8UC1, 
                           get_frame_data(frame, 0), get_frame_linesize(frame, 0));
            y_plane.copyTo(current_gray);
            
        } else {
            // Unsupported format
            STABILIZER_LOG_WARNING( "Unsupported video format for stabilization: %d", get_frame_format(frame));
            result.success = false;
            return result;
        }
        
        // First frame initialization
        if (status_ == StabilizerStatus::INITIALIZING) {
            current_gray.copyTo(previous_gray_);
            
            // Validate frame dimensions for feature detection
            if (current_gray.rows < 50 || current_gray.cols < 50) {
                STABILIZER_LOG_WARNING( "Frame too small for reliable feature detection: %dx%d", 
                        current_gray.cols, current_gray.rows);
                result.success = false;
                return result;
            }
            
            // Detect initial feature points
            if (!detect_features(current_gray)) {
                status_ = StabilizerStatus::ERROR_RECOVERY;
                result.success = false;
                return result;
            }
            
            status_ = StabilizerStatus::ACTIVE;
            STABILIZER_LOG_INFO( "Stabilization initialized with %zu feature points", previous_points_.size());
            result.success = true;
            result.transform_matrix.set_identity();
            return result;
        }
        
        // Track features and calculate transformation
        if (!track_features(current_gray)) {
            handle_tracking_failure();
            result.success = false;
            return result;
        }
        
        // Calculate transformation matrix
        TransformMatrix transform = calculate_transform(previous_points_, current_points_);
        if (transform.is_empty()) {
            handle_tracking_failure();
            result.success = false;
            return result;
        }
        
        // Apply smoothing
        TransformMatrix smoothed_transform = smooth_transform(transform);
        
        // Update state for next frame
        current_gray.copyTo(previous_gray_);
        previous_points_ = current_points_;
        
        // Check if we need to refresh feature points
        frames_since_detection_++;
        if (frames_since_detection_ >= active_config_.refresh_threshold || previous_points_.size() < 50) {
            if (!detect_features(current_gray)) {
                STABILIZER_LOG_WARNING( "Feature refresh failed, continuing with existing points");
            } else {
                frames_since_detection_ = 0;
            }
        }
        
        // Calculate processing time
        auto end_time = std::chrono::high_resolution_clock::now();
        float processing_time = std::chrono::duration<float, std::milli>(end_time - start_time).count();
        
        result.success = true;
        result.transform_matrix = smoothed_transform;
        result.metrics.tracked_features = static_cast<uint32_t>(previous_points_.size());
        result.metrics.processing_time_ms = processing_time;
        result.metrics.status = status_;
        
        update_metrics(result, processing_time);
        update_detailed_metrics(result.metrics);
        consecutive_failures_ = 0;
        
        return result;
        
    } catch (const cv::Exception& e) {
        STABILIZER_LOG_ERROR( "OpenCV error in frame processing: %s", e.what());
        escalate_error();
        result.success = false;
        return result;
    } catch (const std::exception& e) {
        STABILIZER_LOG_ERROR( "Error in frame processing: %s", e.what());
        escalate_error();
        result.success = false;
        return result;
    }
}

void StabilizerCore::update_configuration(const StabilizerConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    active_config_ = config;
    config_dirty_ = true;
}

StabilizerStatus StabilizerCore::get_status() const {
    return status_.load();
}

StabilizerMetrics StabilizerCore::get_metrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return current_metrics_;
}

void StabilizerCore::reset() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    previous_points_.clear();
    current_points_.clear();
    transform_history_buffer_.clear();
    previous_gray_ = cv::Mat();
    
    history_index_ = 0;
    history_filled_ = false;
    frames_since_detection_ = 0;
    consecutive_failures_ = 0;
    
    status_ = StabilizerStatus::INACTIVE;
    
    STABILIZER_LOG_INFO( "StabilizerCore reset");
}

// Private methods implementation

bool StabilizerCore::detect_features(const cv::Mat& gray_frame) {
    return SAFE_BOOL_EXECUTE([&]() -> bool {
        previous_points_.clear();
        
        // SIMD optimization: Ensure proper memory alignment for OpenCV SIMD operations
        auto aligned_frame_guard = [&]() -> CVMatGuard {
            if (gray_frame.isContinuous() && (reinterpret_cast<uintptr_t>(gray_frame.data) % 32 == 0)) {
                return make_mat_guard(gray_frame);
            } else {
                cv::Mat aligned_copy;
                gray_frame.copyTo(aligned_copy);
                return make_mat_guard(std::move(aligned_copy));
            }
        }();
        
        // Adaptive feature detection based on frame resolution
        int optimal_features = std::max(50, std::min(active_config_.max_features, 
                                      static_cast<int>(gray_frame.rows * gray_frame.cols / 10000)));
        
        cv::goodFeaturesToTrack(aligned_frame_guard.get(), previous_points_, 
                               optimal_features, 
                               active_config_.min_feature_quality, 10);
        
        if (previous_points_.size() < 50) {
            ErrorHandler::log_warning(ErrorCategory::FEATURE_DETECTION, 
                                    "detect_features", 
                                    "Insufficient feature points detected");
            return false;
        }
        
        return true;
    }, ErrorCategory::FEATURE_DETECTION, "feature detection");
}

bool StabilizerCore::track_features(const cv::Mat& gray_frame) {
    return ErrorHandler::safe_execute_bool([&]() -> bool {
        if (previous_points_.empty()) {
            return false;
        }
        
        // Pre-allocate vectors to avoid dynamic allocation during tracking
        std::vector<uchar> status;
        std::vector<float> errors;
        status.reserve(previous_points_.size());
        errors.reserve(previous_points_.size());
        
        // Memory alignment optimization for SIMD operations
        cv::Mat aligned_current, aligned_previous;
        if (gray_frame.isContinuous() && previous_gray_.isContinuous()) {
            aligned_current = gray_frame;
            aligned_previous = previous_gray_;
        } else {
            gray_frame.copyTo(aligned_current);
            previous_gray_.copyTo(aligned_previous);
        }
        
        cv::calcOpticalFlowPyrLK(aligned_previous, aligned_current, previous_points_, 
                                current_points_, status, errors);
        
        // Filter good points (pre-allocate for performance)
        std::vector<cv::Point2f> good_prev, good_current;
        good_prev.reserve(previous_points_.size());
        good_current.reserve(previous_points_.size());
        
        for (size_t i = 0; i < status.size(); ++i) {
            if (status[i] && errors[i] < active_config_.error_threshold) {
                good_prev.push_back(previous_points_[i]);
                good_current.push_back(current_points_[i]);
            }
        }
        
        if (good_prev.size() < 10) {
            ErrorHandler::log_warning(ErrorCategory::FEATURE_TRACKING, 
                                    "track_features", 
                                    "Too few good tracking points");
            return false;
        }
        
        previous_points_ = good_prev;
        current_points_ = good_current;
        
        return true;
    }, ErrorCategory::FEATURE_TRACKING, "feature tracking");
}

TransformMatrix StabilizerCore::calculate_transform(const std::vector<cv::Point2f>& prev_pts,
                                                  const std::vector<cv::Point2f>& curr_pts) {
    cv::Mat opencv_result;
    if (!SAFE_CV_EXECUTE([&]() {
        // Unified parameter validation
        auto prev_validation = ParameterValidator::validate_feature_points(prev_pts, 4, "Previous feature points");
        auto curr_validation = ParameterValidator::validate_feature_points(curr_pts, 4, "Current feature points");
        
        if (!prev_validation || !curr_validation) {
            return cv::Mat();
        }
        
        cv::Mat transform = cv::estimateAffinePartial2D(prev_pts, curr_pts);
        
        if (transform.empty()) {
            return cv::Mat();
        }
        
        // Validate transform matrix using unified validation
        auto transform_validation = ParameterValidator::validate_transform_matrix(transform);
        if (!transform_validation) {
            ErrorHandler::log_warning(ErrorCategory::TRANSFORM_CALCULATION, 
                                    "calculate_transform", 
                                    transform_validation.error_message);
            return cv::Mat();
        }
        
        return transform;
    }, opencv_result, ErrorCategory::TRANSFORM_CALCULATION, "transform calculation")) {
        return TransformMatrix(); // Returns empty TransformMatrix
    }
    
    // Convert cv::Mat to TransformMatrix
    return TransformMatrix(opencv_result);
}

TransformMatrix StabilizerCore::smooth_transform(const TransformMatrix& transform) {
    // Add transform to history
    transform_history_buffer_[history_index_] = transform;
    history_index_ = (history_index_ + 1) % active_config_.smoothing_radius;
    
    if (!history_filled_ && history_index_ == 0) {
        history_filled_ = true;
    }
    
    // Calculate moving average using transform_utils
    int count = history_filled_ ? active_config_.smoothing_radius : history_index_;
    
    std::vector<TransformMatrix> transforms_for_average;
    transforms_for_average.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        transforms_for_average.push_back(transform_history_buffer_[i]);
    }
    
    return transform_utils::average_transforms(transforms_for_average);
}

void StabilizerCore::apply_configuration_if_dirty() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    if (config_dirty_.exchange(false)) {
        
        // Resize transform history if smoothing radius changed
        if (transform_history_buffer_.size() != static_cast<size_t>(active_config_.smoothing_radius)) {
            transform_history_buffer_.clear();
            transform_history_buffer_.reserve(active_config_.smoothing_radius);
            
            for (int i = 0; i < active_config_.smoothing_radius; ++i) {
                transform_history_buffer_.emplace_back(); // Creates identity TransformMatrix
            }
            
            history_index_ = 0;
            history_filled_ = false;
        }
        
        STABILIZER_LOG_INFO( "Configuration updated: smoothing=%d, features=%d", 
                active_config_.smoothing_radius, active_config_.max_features);
    }
}

void StabilizerCore::update_metrics(const TransformResult& result, float processing_time) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    current_metrics_.tracked_features = result.metrics.tracked_features;
    current_metrics_.processing_time_ms = processing_time;
    current_metrics_.status = status_;
    current_metrics_.error_count = consecutive_failures_;
    
    // Calculate transform stability (simplified metric)
    double dx = result.transform_matrix.get_translation_x();
    double dy = result.transform_matrix.get_translation_y();
    current_metrics_.transform_stability = std::max(0.0f, 1.0f - static_cast<float>(sqrt(dx*dx + dy*dy) / 100.0));
}

void StabilizerCore::handle_tracking_failure() {
    consecutive_failures_++;
    
    if (consecutive_failures_ > 5) {
        escalate_error();
    } else {
        // Try to recover by re-detecting features
        if (!previous_gray_.empty()) {
            detect_features(previous_gray_);
        }
        status_ = StabilizerStatus::ERROR_RECOVERY;
    }
}

void StabilizerCore::escalate_error() {
    consecutive_failures_++;
    
    if (consecutive_failures_ > 10) {
        status_ = StabilizerStatus::FAILED;
        STABILIZER_LOG_ERROR( "Stabilization failed after multiple consecutive errors");
    } else {
        status_ = StabilizerStatus::DEGRADED;
        STABILIZER_LOG_WARNING( "Stabilization degraded due to tracking failures");
    }
}

#endif // ENABLE_STABILIZATION

} // namespace obs_stabilizer
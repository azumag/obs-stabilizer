/*
OBS Stabilizer Plugin - Core Stabilization Engine Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "stabilizer_core.hpp"
#include <obs-module.h>
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
    
    try {
        active_config_ = config;
        
        // Initialize transform history buffer
        transform_history_.clear();
        transform_history_.reserve(active_config_.smoothing_radius);
        
        // Initialize OpenCV matrices
        cv::Mat identity = cv::Mat::eye(2, 3, CV_64F);
        for (int i = 0; i < active_config_.smoothing_radius; ++i) {
            transform_history_.push_back(identity.clone());
        }
        
        history_index_ = 0;
        history_filled_ = false;
        frames_since_detection_ = 0;
        consecutive_failures_ = 0;
        
        status_ = StabilizerStatus::INITIALIZING;
        obs_log(LOG_INFO, "StabilizerCore initialized (smoothing=%d, features=%d)", 
                active_config_.smoothing_radius, active_config_.max_features);
        
        return true;
        
    } catch (const std::exception& e) {
        status_ = StabilizerStatus::FAILED;
        obs_log(LOG_ERROR, "StabilizerCore initialization failed: %s", e.what());
        return false;
    }
}

TransformResult StabilizerCore::process_frame(struct obs_source_frame* frame) {
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
        // Comprehensive input validation
        if (!frame || !frame->data[0] || frame->width == 0 || frame->height == 0) {
            obs_log(LOG_ERROR, "Invalid frame data for stabilization");
            result.success = false;
            return result;
        }
        
        // Validate frame dimensions and prevent integer overflow
        if (frame->width > 8192 || frame->height > 8192) {
            obs_log(LOG_ERROR, "Frame dimensions too large: %ux%u", frame->width, frame->height);
            result.success = false;
            return result;
        }
        
        // Convert frame to grayscale for processing
        cv::Mat current_gray;
        if (frame->format == VIDEO_FORMAT_NV12) {
            // Validate NV12 format requirements
            if (!frame->data[0] || frame->linesize[0] < frame->width) {
                obs_log(LOG_ERROR, "Invalid NV12 frame data or linesize");
                result.success = false;
                return result;
            }
            
            // Use Y plane directly
            cv::Mat nv12_y(frame->height, frame->width, CV_8UC1, 
                          frame->data[0], frame->linesize[0]);
            nv12_y.copyTo(current_gray);
            
        } else if (frame->format == VIDEO_FORMAT_I420) {
            // Validate I420 format requirements  
            if (!frame->data[0] || frame->linesize[0] < frame->width) {
                obs_log(LOG_ERROR, "Invalid I420 frame data or linesize");
                result.success = false;
                return result;
            }
            
            // Use Y plane directly
            cv::Mat y_plane(frame->height, frame->width, CV_8UC1, 
                           frame->data[0], frame->linesize[0]);
            y_plane.copyTo(current_gray);
            
        } else {
            // Unsupported format
            obs_log(LOG_WARNING, "Unsupported video format for stabilization: %d", frame->format);
            result.success = false;
            return result;
        }
        
        // First frame initialization
        if (status_ == StabilizerStatus::INITIALIZING) {
            current_gray.copyTo(previous_gray_);
            
            // Validate frame dimensions for feature detection
            if (current_gray.rows < 50 || current_gray.cols < 50) {
                obs_log(LOG_WARNING, "Frame too small for reliable feature detection: %dx%d", 
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
            obs_log(LOG_INFO, "Stabilization initialized with %zu feature points", previous_points_.size());
            result.success = true;
            result.transform_matrix = cv::Mat::eye(2, 3, CV_64F);
            return result;
        }
        
        // Track features and calculate transformation
        if (!track_features(current_gray)) {
            handle_tracking_failure();
            result.success = false;
            return result;
        }
        
        // Calculate transformation matrix
        cv::Mat transform = calculate_transform(previous_points_, current_points_);
        if (transform.empty()) {
            handle_tracking_failure();
            result.success = false;
            return result;
        }
        
        // Apply smoothing
        cv::Mat smoothed_transform = smooth_transform(transform);
        
        // Update state for next frame
        current_gray.copyTo(previous_gray_);
        previous_points_ = current_points_;
        
        // Check if we need to refresh feature points
        frames_since_detection_++;
        if (frames_since_detection_ >= active_config_.refresh_threshold || previous_points_.size() < 50) {
            if (!detect_features(current_gray)) {
                obs_log(LOG_WARNING, "Feature refresh failed, continuing with existing points");
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
        consecutive_failures_ = 0;
        
        return result;
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "OpenCV error in frame processing: %s", e.what());
        escalate_error();
        result.success = false;
        return result;
    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Error in frame processing: %s", e.what());
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
    transform_history_.clear();
    previous_gray_ = cv::Mat();
    
    history_index_ = 0;
    history_filled_ = false;
    frames_since_detection_ = 0;
    consecutive_failures_ = 0;
    
    status_ = StabilizerStatus::INACTIVE;
    
    obs_log(LOG_INFO, "StabilizerCore reset");
}

// Private methods implementation

bool StabilizerCore::detect_features(const cv::Mat& gray_frame) {
    try {
        previous_points_.clear();
        cv::goodFeaturesToTrack(gray_frame, previous_points_, 
                               std::max(50, std::min(active_config_.max_features, 1000)), 
                               active_config_.min_feature_quality, 10);
        
        if (previous_points_.size() < 50) {
            obs_log(LOG_WARNING, "Insufficient feature points detected: %zu", previous_points_.size());
            return false;
        }
        
        return true;
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "Feature detection failed: %s", e.what());
        return false;
    }
}

bool StabilizerCore::track_features(const cv::Mat& gray_frame) {
    try {
        if (previous_points_.empty()) {
            return false;
        }
        
        std::vector<uchar> status;
        std::vector<float> errors;
        
        cv::calcOpticalFlowPyrLK(previous_gray_, gray_frame, previous_points_, 
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
            obs_log(LOG_WARNING, "Too few good tracking points: %zu", good_prev.size());
            return false;
        }
        
        previous_points_ = good_prev;
        current_points_ = good_current;
        
        return true;
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "Feature tracking failed: %s", e.what());
        return false;
    }
}

cv::Mat StabilizerCore::calculate_transform(const std::vector<cv::Point2f>& prev_pts,
                                           const std::vector<cv::Point2f>& curr_pts) {
    try {
        if (prev_pts.size() < 4 || curr_pts.size() < 4) {
            return cv::Mat();
        }
        
        cv::Mat transform = cv::estimateAffinePartial2D(prev_pts, curr_pts);
        
        if (transform.empty()) {
            return cv::Mat();
        }
        
        // Validate transform matrix for reasonable values
        double dx = transform.at<double>(0, 2);
        double dy = transform.at<double>(1, 2);
        double scale = sqrt(transform.at<double>(0, 0) * transform.at<double>(0, 0) + 
                           transform.at<double>(0, 1) * transform.at<double>(0, 1));
        
        // Reject unreasonable transformations
        if (abs(dx) > 100 || abs(dy) > 100 || scale < 0.5 || scale > 2.0) {
            obs_log(LOG_WARNING, "Rejecting unreasonable transform: dx=%.2f, dy=%.2f, scale=%.2f", 
                    dx, dy, scale);
            return cv::Mat();
        }
        
        return transform;
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "Transform calculation failed: %s", e.what());
        return cv::Mat();
    }
}

cv::Mat StabilizerCore::smooth_transform(const cv::Mat& transform) {
    // Add transform to history
    transform.copyTo(transform_history_[history_index_]);
    history_index_ = (history_index_ + 1) % active_config_.smoothing_radius;
    
    if (!history_filled_ && history_index_ == 0) {
        history_filled_ = true;
    }
    
    // Calculate moving average
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    int count = history_filled_ ? active_config_.smoothing_radius : history_index_;
    
    for (int i = 0; i < count; ++i) {
        smoothed += transform_history_[i];
    }
    
    smoothed /= count;
    return smoothed;
}

void StabilizerCore::apply_configuration_if_dirty() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    if (config_dirty_.exchange(false)) {
        
        // Resize transform history if smoothing radius changed
        if (transform_history_.size() != static_cast<size_t>(active_config_.smoothing_radius)) {
            cv::Mat identity = cv::Mat::eye(2, 3, CV_64F);
            transform_history_.clear();
            transform_history_.reserve(active_config_.smoothing_radius);
            
            for (int i = 0; i < active_config_.smoothing_radius; ++i) {
                transform_history_.push_back(identity.clone());
            }
            
            history_index_ = 0;
            history_filled_ = false;
        }
        
        obs_log(LOG_INFO, "Configuration updated: smoothing=%d, features=%d", 
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
    double dx = result.transform_matrix.at<double>(0, 2);
    double dy = result.transform_matrix.at<double>(1, 2);
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
        obs_log(LOG_ERROR, "Stabilization failed after multiple consecutive errors");
    } else {
        status_ = StabilizerStatus::DEGRADED;
        obs_log(LOG_WARNING, "Stabilization degraded due to tracking failures");
    }
}

#endif // ENABLE_STABILIZATION

} // namespace obs_stabilizer
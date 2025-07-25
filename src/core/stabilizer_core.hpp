/*
OBS Stabilizer Plugin - Core Stabilization Engine
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#endif

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

// Forward declarations
struct obs_source_frame;

namespace obs_stabilizer {

// Configuration structure for stabilizer parameters
struct StabilizerConfig {
    // Core parameters
    int smoothing_radius = 30;        // Range: 10-100
    int max_features = 200;           // Range: 100-1000  
    float error_threshold = 30.0f;    // Range: 10.0-100.0
    
    // Processing options
    bool enable_stabilization = true;
    
    // Advanced parameters
    float min_feature_quality = 0.01f;
    int refresh_threshold = 25;
    bool adaptive_refresh = true;
};

// Status enumeration for stabilizer state
enum class StabilizerStatus {
    INACTIVE,           // Stabilization disabled
    INITIALIZING,       // First frame processing
    ACTIVE,            // Normal operation
    DEGRADED,          // Reduced quality mode
    ERROR_RECOVERY,    // Recovering from error
    FAILED            // Stabilization failed
};

// Metrics structure for performance monitoring
struct StabilizerMetrics {
    uint32_t tracked_features = 0;
    float processing_time_ms = 0.0f;
    float transform_stability = 0.0f;
    uint32_t error_count = 0;
    StabilizerStatus status = StabilizerStatus::INACTIVE;
};

#ifdef ENABLE_STABILIZATION
// Transform result structure (OpenCV version)
struct TransformResult {
    bool success = false;
    cv::Mat transform_matrix;
    StabilizerMetrics metrics;
};
#else
// Transform result structure (stub version)
struct TransformResult {
    bool success = false;
    void* transform_matrix = nullptr;  // Placeholder for cv::Mat
    StabilizerMetrics metrics;
};
#endif

#ifdef ENABLE_STABILIZATION
// Core stabilization engine class
class StabilizerCore {
public:
    StabilizerCore();
    ~StabilizerCore();

    // Initialize stabilizer with configuration
    bool initialize(const StabilizerConfig& config);
    
    // Process a video frame and return transformation result
    TransformResult process_frame(struct obs_source_frame* frame);
    
    // Update configuration (thread-safe)
    void update_configuration(const StabilizerConfig& config);
    
    // Get current status and metrics
    StabilizerStatus get_status() const;
    StabilizerMetrics get_metrics() const;
    
    // Reset stabilizer state
    void reset();

private:
    // Thread-safe configuration management
    mutable std::mutex config_mutex_;
    StabilizerConfig active_config_;
    std::atomic<bool> config_dirty_{false};
    
    // OpenCV processing state
    std::vector<cv::Point2f> previous_points_;
    std::vector<cv::Point2f> current_points_;
    cv::Mat previous_gray_;
    
    // Transform smoothing
    std::vector<cv::Mat> transform_history_;
    size_t history_index_ = 0;
    bool history_filled_ = false;
    
    // Status and metrics
    std::atomic<StabilizerStatus> status_{StabilizerStatus::INACTIVE};
    StabilizerMetrics current_metrics_;
    mutable std::mutex metrics_mutex_;
    
    // Frame tracking state
    int frames_since_detection_ = 0;
    int consecutive_failures_ = 0;
    
    // Internal processing methods
    bool detect_features(const cv::Mat& gray_frame);
    bool track_features(const cv::Mat& gray_frame);
    cv::Mat calculate_transform(const std::vector<cv::Point2f>& prev_pts,
                               const std::vector<cv::Point2f>& curr_pts);
    cv::Mat smooth_transform(const cv::Mat& transform);
    void apply_configuration_if_dirty();
    void update_metrics(const TransformResult& result, float processing_time);
    
    // Error recovery
    void handle_tracking_failure();
    void escalate_error();
};
#else
// Stub implementation when OpenCV is not available
class StabilizerCore {
public:
    StabilizerCore() = default;
    ~StabilizerCore() = default;
    
    bool initialize(const StabilizerConfig&) { return false; }
    TransformResult process_frame(struct obs_source_frame*) { 
        return TransformResult{false, nullptr, StabilizerMetrics{}}; 
    }
    void update_configuration(const StabilizerConfig&) {}
    StabilizerStatus get_status() const { return StabilizerStatus::INACTIVE; }
    StabilizerMetrics get_metrics() const { return StabilizerMetrics{}; }
    void reset() {}
};
#endif

} // namespace obs_stabilizer
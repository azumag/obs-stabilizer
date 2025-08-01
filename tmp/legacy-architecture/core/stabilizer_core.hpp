/*
OBS Stabilizer Plugin - Core Stabilization Engine
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include "transform_matrix.hpp"

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#endif

#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

// Forward declarations and frame abstraction
#include "frame_adapter.hpp"

namespace obs_stabilizer {

// Configuration structure for stabilizer parameters
struct StabilizerConfig {
    // Core stabilization parameters
    int smoothing_radius = 30;        // Range: 10-100, UI: Slider
    int max_features = 200;           // Range: 100-1000, UI: Number input
    float error_threshold = 30.0f;    // Range: 10.0-100.0, UI: Slider

    // User-facing toggles
    bool enable_stabilization = true; // UI: Checkbox

    // Advanced parameters (collapsible section)
    float min_feature_quality = 0.01f;  // Range: 0.001-0.1, UI: Advanced slider
    int refresh_threshold = 25;         // Range: 10-50, UI: Advanced number
    bool adaptive_refresh = true;       // UI: Advanced checkbox

    // Output options
    enum class OutputMode {
        CROP,           // Crop to remove black borders (default)
        PAD,            // Pad with black borders to maintain size
        SCALE_FIT       // Scale to fit, may cause slight distortion
    };
    OutputMode output_mode = OutputMode::CROP; // UI: Radio buttons

    // Preset system
    enum class PresetMode {
        CUSTOM,         // User-defined settings
        GAMING,         // Optimized for gaming captures (fast response)
        STREAMING,      // Balanced for live streaming (medium quality)
        RECORDING       // High quality for post-production (slow response)
    };
    PresetMode preset = PresetMode::STREAMING; // UI: Dropdown

    // Performance settings
    bool enable_gpu_acceleration = false;  // UI: Advanced checkbox
    int processing_threads = 1;            // Range: 1-8, UI: Advanced slider
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

// Metrics structure for performance monitoring and debugging
struct StabilizerMetrics {
    uint32_t tracked_features = 0;
    float processing_time_ms = 0.0f;

    // Enhanced diagnostic metrics
    float feature_detection_time_ms = 0.0f;
    float optical_flow_time_ms = 0.0f;
    float transform_calc_time_ms = 0.0f;
    float smoothing_time_ms = 0.0f;

    // Quality metrics
    float tracking_success_rate = 0.0f;
    uint32_t features_lost = 0;
    uint32_t features_refreshed = 0;
    float transform_stability = 0.0f;
    uint32_t error_count = 0;
    StabilizerStatus status = StabilizerStatus::INACTIVE;
};

// Transform result structure (unified for OpenCV and stub modes)
struct TransformResult {
    bool success = false;
    TransformMatrix transform_matrix;
    StabilizerMetrics metrics;
};

#ifdef ENABLE_STABILIZATION
// Core stabilization engine class
//
// THREAD SAFETY NOTES:
// - This class is designed to be used from the OBS video processing thread
// - Configuration updates (update_configuration) are thread-safe via config_mutex_
// - Metrics access (get_metrics) is thread-safe via metrics_mutex_
// - process_frame() should only be called from one thread at a time
// - reset() is thread-safe but should not be called during frame processing
// - All OpenCV operations are contained within this class for thread isolation
class StabilizerCore {
public:
    StabilizerCore();
    ~StabilizerCore();

    // Initialize stabilizer with configuration
    bool initialize(const StabilizerConfig& config);

    // Process a video frame and return transformation result
    TransformResult process_frame(frame_t* frame);

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
    std::vector<TransformMatrix> transform_history_buffer_;
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
    TransformMatrix calculate_transform(const std::vector<cv::Point2f>& prev_pts,
                                       const std::vector<cv::Point2f>& curr_pts);

    // Debug and diagnostic methods
    void update_detailed_metrics(const StabilizerMetrics& frame_metrics);
    void log_performance_breakdown() const;
    TransformMatrix smooth_transform(const TransformMatrix& transform);
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
    TransformResult process_frame(frame_t*) {
        return TransformResult{false, TransformMatrix{}, StabilizerMetrics{}};
    }
    void update_configuration(const StabilizerConfig&) {}
    StabilizerStatus get_status() const { return StabilizerStatus::INACTIVE; }
    StabilizerMetrics get_metrics() const { return StabilizerMetrics{}; }
    void reset() {}
};
#endif

} // namespace obs_stabilizer
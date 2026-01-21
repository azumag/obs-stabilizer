/*
 * OBS Stabilizer Core Module
 * Implements the core stabilization algorithms using OpenCV
 * Separated from OBS integration for modularity and testability
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/features2d.hpp>

using namespace cv;
#include <mutex>
#include <memory>
#include <deque>
#include <vector>
#include <chrono>
#include "../stabilizer_constants.h"

// Platform-specific optimizations (disabled in standalone build)
#ifndef BUILD_STANDALONE
#include "platform_optimization.hpp"
#endif

/**
 * Core stabilization engine that processes video frames
 * Implements Lucas-Kanade optical flow for real-time stabilization
 */
class StabilizerCore {
public:
    struct StabilizerParams {
        bool enabled = true;
        int smoothing_radius = 30;         // Number of frames to average for smoothing
        float max_correction = 30.0f;      // Maximum correction percentage
        int feature_count = 500;            // Number of feature points to track
        float quality_level = 0.01f;       // Minimal accepted quality of corners
        float min_distance = 30.0f;        // Minimum distance between corners
        int block_size = 3;                // Block size for derivative covariation matrix
        bool use_harris = false;           // Use Harris detector
        float k = 0.04f;                   // Harris detector parameter
        bool debug_mode = false;           // Enable debug output

        // Motion thresholds and limits
        float frame_motion_threshold = 0.25f; // Motion threshold to trigger stabilization
        float max_displacement = 1000.0f;     // Maximum allowed feature displacement
        double tracking_error_threshold = 50.0; // LK tracking error threshold
        
        // RANSAC parameters
        float ransac_threshold_min = 1.0f;
        float ransac_threshold_max = 10.0f;
        
        // Point validation
        float min_point_spread = 10.0f;       // Minimum spread of feature points
        float max_coordinate = 100000.0f;      // Maximum valid coordinate value

        // Algorithm optimization parameters (Phase 2)
        int optical_flow_pyramid_levels = 3;   // Pyramid levels for optical flow (0-5)
        int optical_flow_window_size = 21;       // Search window size (must be odd, 5-31)
        float feature_refresh_threshold = 0.5f;   // Refresh features when tracking success < this (0.0-1.0)
        int adaptive_feature_min = 100;          // Minimum adaptive feature count
        int adaptive_feature_max = 500;          // Maximum adaptive feature count
        
        // Motion-specific smoothing parameters (Phase 4)
        bool use_high_pass_filter = false;     // Enable high-pass filter for camera shake
        double high_pass_attenuation = 0.3;    // High-frequency attenuation factor (0.0-1.0)
        bool use_directional_smoothing = false; // Enable directional smoothing for pan/zoom
    };

    struct PerformanceMetrics {
        double avg_processing_time = 0.0;
        uint64_t frame_count = 0;
        std::chrono::high_resolution_clock::time_point last_frame_time;
    };

    /**
     * Initialize the stabilizer core
     * @param width Frame width
     * @param height Frame height
     * @param params Stabilization parameters
     * @return True if initialization successful
     */
    bool initialize(uint32_t width, uint32_t height, const StabilizerParams& params);

    /**
     * Process a single video frame
     * @param frame Input frame in BGRA format
     * @return Processed frame (stabilized) or nullptr on error
     */
    cv::Mat process_frame(const cv::Mat& frame);

    /**
     * Update stabilization parameters
     * @param params New parameters
     */
    void update_parameters(const StabilizerParams& params);

    /**
     * Reset the stabilizer state (clear previous frame data)
     */
    void reset();

    /**
     * @brief Clear all internal state
     */
    void clear_state();
    
    /**
     * Get current performance metrics
     * @return Performance metrics structure
     */
    PerformanceMetrics get_performance_metrics() const;
    
    /**
     * Get current transform history
     * @return Reference to transform deque
     */
    const std::deque<cv::Mat>& get_current_transforms() const;
    
    /**
     * Motion-specific smoothing algorithms for adaptive stabilization
     */
    cv::Mat smooth_high_pass_filter(const std::deque<cv::Mat>& transforms, double attenuation = 0.3);
    cv::Mat smooth_directional(const std::deque<cv::Mat>& transforms, const cv::Vec2d& direction);

    /**
     * Check if stabilizer is ready for processing
     * @return True if ready
     */
    bool is_ready() const;

    /**
     * Get last error message
     * @return Error string or empty if no error
     */
    std::string get_last_error() const;

    /**
     * Get current parameters
     * @return The current stabilizer parameters
     */
    StabilizerParams get_current_params() const;

    // Parameter validation
    static bool validate_parameters(const StabilizerParams& params);

    // Preset configurations
    static StabilizerParams get_preset_gaming();
    static StabilizerParams get_preset_streaming();
    static StabilizerParams get_preset_recording();

    bool validate_frame(const cv::Mat& frame);

private:
    // Core algorithm implementation
    bool detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points);
    bool track_features(const cv::Mat& prev_gray, const cv::Mat& curr_gray,
                      std::vector<cv::Point2f>& prev_pts, std::vector<cv::Point2f>& curr_pts,
                      float& success_rate);
    cv::Mat estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                              std::vector<cv::Point2f>& curr_pts);
    cv::Mat smooth_transforms();
    cv::Mat apply_transform(const cv::Mat& frame, const cv::Mat& transform);

    // Optimized inline functions for performance-critical paths
    inline cv::Mat smooth_transforms_optimized();
    inline void filter_transforms(std::vector<cv::Mat>& transforms);
    inline bool should_refresh_features(float success_rate, int frames_since_refresh);
    inline void update_metrics(const std::chrono::high_resolution_clock::time_point& start_time);

    // Internal state
    mutable std::mutex mutex_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool first_frame_ = true;

    StabilizerParams params_;

    // OpenCV data structures
    cv::Mat prev_gray_;
    std::vector<cv::Point2f> prev_pts_;
    std::deque<cv::Mat> transforms_;
    cv::Mat cumulative_transform_;

    // Performance monitoring
    PerformanceMetrics metrics_;

    // Error handling
    std::string last_error_;

    // Algorithm optimization state (Phase 2)
    int consecutive_tracking_failures_ = 0;
    int frames_since_last_refresh_ = 0;

    // Named constants for magic numbers
    static constexpr int MIN_FEATURES_FOR_TRACKING = 4;
    static constexpr int MAX_POINTS_TO_PROCESS = 1000;
    static constexpr int MIN_IMAGE_SIZE = 32;
    static constexpr int MAX_IMAGE_WIDTH = 7680;
    static constexpr int MAX_IMAGE_HEIGHT = 4320;
    static constexpr double MAX_TRANSFORM_SCALE = 100.0;
    static constexpr double MAX_TRANSLATION = 2000.0;
    static constexpr double TRACKING_ERROR_THRESHOLD = 50.0;
};

/**
 * Type-safe wrapper for transformation matrices
 * Provides thread-safe operations and validation
 */
class TransformMatrix {
public:
    TransformMatrix();
    explicit TransformMatrix(const cv::Mat& matrix);
    
    // Matrix operations
    TransformMatrix operator*(const TransformMatrix& other) const;
    TransformMatrix inverse() const;
    
    // Accessors
    cv::Mat get_matrix() const { return matrix_; }
    bool is_valid() const;
    
    // Utility methods
    static TransformMatrix identity();
    static TransformMatrix from_affine(double tx, double ty, double angle, double scale);
    
private:
    cv::Mat matrix_;
    mutable std::mutex mutex_;
};

/**
 * Error handling utilities
 */
class ErrorHandler {
public:
    enum class ErrorType {
        Initialization,
        Memory,
        OpenCV,
        Parameter,
        Processing,
        Thread
    };
    
    static std::string format_error(ErrorType type, const std::string& message);
    static void log_error(ErrorType type, const std::string& message);
    static std::string get_last_error();
    
private:
    static thread_local std::string last_error_;
};
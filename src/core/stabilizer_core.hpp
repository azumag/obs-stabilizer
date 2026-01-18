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
#include <mutex>
#include <memory>
#include <deque>
#include <vector>
#include <chrono>
#include "../stabilizer_constants.h"

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
     * Get current performance metrics
     * @return Performance metrics structure
     */
    PerformanceMetrics get_performance_metrics() const;

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

    // Parameter validation
    static bool validate_parameters(const StabilizerParams& params);

    // Preset configurations
    static StabilizerParams get_preset_gaming();
    static StabilizerParams get_preset_streaming();
    static StabilizerParams get_preset_recording();

private:
    // Core algorithm implementation
    bool detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points);
    bool track_features(const cv::Mat& prev_gray, const cv::Mat& curr_gray,
                      std::vector<cv::Point2f>& prev_pts, std::vector<cv::Point2f>& curr_pts);
    cv::Mat estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                              const std::vector<cv::Point2f>& curr_pts);
    cv::Mat smooth_transforms();
    cv::Mat apply_transform(const cv::Mat& frame, const cv::Mat& transform);

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

    // Utility methods
    void log_performance(double processing_time);
    bool validate_frame(const cv::Mat& frame);
    void clear_state();
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
 * Parameter validation utility
 */
class ParameterValidator {
public:
    static bool validate_smoothing_radius(int value);
    static bool validate_max_correction(float value);
    static bool validate_feature_count(int value);
    static bool validate_quality_level(float value);
    static bool validate_min_distance(float value);
    static bool validate_block_size(int value);
    static bool validate_harris_k(float value);
    
    static bool validate_all(const StabilizerCore::StabilizerParams& params);
    static std::vector<std::string> get_validation_errors(const StabilizerCore::StabilizerParams& params);
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
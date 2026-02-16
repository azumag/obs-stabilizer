/*
 * OBS Stabilizer Core Module
 * Implements the core stabilization algorithms using OpenCV
 * Separated from OBS integration for modularity and testability
 *
 * DESIGN NOTE: StabilizerCore is intentionally single-threaded (no mutex)
 * Thread safety is provided by StabilizerWrapper layer above.
 * This separation keeps the core algorithm simple and performant.
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/features2d.hpp>

#include <memory>
#include <deque>
#include <vector>
#include <chrono>
#include "stabilizer_constants.hpp"

/**
 * Core stabilization engine that processes video frames
 * Implements Lucas-Kanade optical flow for real-time stabilization
 *
 * DESIGN PRINCIPLE: Single-threaded design for performance
 * - No mutex locking in the processing path
 * - Thread safety is handled by StabilizerWrapper (caller's responsibility)
 * - This keeps the core algorithm simple and fast (KISS principle)
 */
class StabilizerCore {
    // Forward declaration for test access
    friend class StabilizerCoreTest;

public:
    enum class EdgeMode {
        Padding,    // Keep black borders (current behavior)
        Crop,       // Crop borders to remove black areas
        Scale       // Scale to fit original frame
    };

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

        // Edge handling
        EdgeMode edge_mode = EdgeMode::Padding;  // Edge handling mode: Padding, Crop, Scale
    };

    struct PerformanceMetrics {
        double avg_processing_time = 0.0;
        uint64_t frame_count = 0;
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
      * @return Processed frame (stabilized) or empty Mat on error
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
      * Get current transform history
      * @return Reference to transform deque
      */
    const std::deque<cv::Mat>& get_current_transforms() const;

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

    // Preset configurations
    static StabilizerParams get_preset_gaming();
    static StabilizerParams get_preset_streaming();
    static StabilizerParams get_preset_recording();

    bool validate_frame(const cv::Mat& frame);

    /**
     * Detect content bounds in a frame
     * @param frame Input frame to analyze
     * @return Rectangle containing non-black content
     */
    cv::Rect detect_content_bounds(const cv::Mat& frame);

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
    inline void update_metrics(const std::chrono::high_resolution_clock::time_point& start_time);

    // Edge handling
    cv::Mat apply_edge_handling(const cv::Mat& frame, EdgeMode mode);

    // Internal state
    // DESIGN NOTE: No mutex used - StabilizerCore is single-threaded by design
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool first_frame_ = true;

    StabilizerParams params_;

    // OpenCV data structures
    cv::Mat prev_gray_;
    std::vector<cv::Point2f> prev_pts_;
    std::deque<cv::Mat> transforms_;

    // Performance monitoring
    PerformanceMetrics metrics_;

    // Error handling
    std::string last_error_;

    // Algorithm optimization state
    int consecutive_tracking_failures_ = 0;

    // Named constants for magic numbers
    static constexpr int MIN_FEATURES_FOR_TRACKING = 4;
};

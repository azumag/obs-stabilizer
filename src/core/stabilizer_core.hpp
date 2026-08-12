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
#include "kalman_transform_filter.hpp"

/**
 * @brief Core single-threaded video stabilization engine.
 *
 * The engine detects features, tracks them with pyramidal Lucas-Kanade optical
 * flow, estimates inter-frame motion, smooths the recent transform history and
 * applies the resulting correction to each BGRA frame.
 *
 * @par Threading model
 * Instances are intentionally not internally synchronized. Callers must either
 * serialize access or use StabilizerWrapper, which owns the synchronization
 * policy. Keeping locks out of this class avoids contention in the per-frame
 * processing path.
 *
 * @par Performance characteristics
 * Processing cost grows with frame size, feature_count and smoothing_radius.
 * The implementation retains the previous grayscale frame and a bounded
 * transform history, so memory usage is proportional to frame size plus the
 * configured smoothing window.
 *
 * @par Limitations
 * The current public interface uses OpenCV types directly and assumes BGRA
 * input. Large scene cuts, heavy motion blur and frames with too few trackable
 * features may produce an unchanged frame while tracking state is rebuilt.
 */
class StabilizerCore {
    friend class StabilizerCoreTest;

public:
    /** @brief Strategy used to handle borders introduced by stabilization. */
    enum class EdgeMode {
        Padding, ///< Keep exposed borders as black pixels.
        Crop,    ///< Crop away exposed border regions.
        Scale    ///< Scale corrected content to fill the original frame.
    };

    /** @brief Trajectory smoothing strategy. */
    enum class SmoothingMode {
        MovingAverage, ///< Classic windowed average of recent transforms.
        Kalman         ///< Constant-velocity Kalman filter over transform components.
    };

    /** @brief Runtime parameters controlling feature detection and correction. */
    struct StabilizerParams {
        bool enabled = true;                  ///< Disable processing while preserving configuration.
        int smoothing_radius = 30;            ///< Number of recent transforms included in smoothing.
        float max_correction = 30.0f;         ///< Maximum correction as a percentage of frame extent.
        int feature_count = 500;              ///< Maximum number of feature points to detect.
        float quality_level = 0.01f;           ///< Minimum corner quality accepted by goodFeaturesToTrack.
        float min_distance = 30.0f;            ///< Minimum pixel distance between detected corners.
        int block_size = 3;                    ///< Neighborhood size used by corner detection.
        bool use_harris = false;               ///< Use the Harris detector instead of the default score.
        float k = 0.04f;                       ///< Harris detector free parameter.
        bool debug_mode = false;               ///< Enable diagnostic output.
        double tracking_error_threshold = 50.0; ///< Reserved LK error threshold for adaptive stabilization.
        float ransac_threshold_min = 1.0f;     ///< Lower bound for adaptive RANSAC reprojection threshold.
        float ransac_threshold_max = 10.0f;    ///< Upper bound for adaptive RANSAC reprojection threshold.
        float min_point_spread = 10.0f;        ///< Minimum spatial spread required from tracked points.
        float max_coordinate = 100000.0f;      ///< Absolute coordinate limit used to reject invalid points.
        EdgeMode edge_mode = EdgeMode::Padding; ///< Border handling strategy.
        SmoothingMode smoothing_mode = SmoothingMode::MovingAverage; ///< Trajectory smoothing strategy.
    };

    /** @brief Cumulative processing counters for the current initialized state. */
    struct PerformanceMetrics {
        double avg_processing_time = 0.0; ///< Mean processing duration per observed frame, in milliseconds.
        uint64_t total_frames = 0;        ///< Number of calls included in metrics.
        uint64_t successful_frames = 0;   ///< Number of frames stabilized successfully.
        uint64_t tracking_failures = 0;   ///< Number of frames where motion tracking could not be used.
    };

    /**
     * @brief Initialize or reinitialize the stabilizer for a frame geometry.
     * @param width Expected frame width in pixels; must be greater than zero.
     * @param height Expected frame height in pixels; must be greater than zero.
     * @param params Initial stabilization parameters.
     * @return true when initialization succeeds; false when geometry or
     *         parameters are invalid.
     *
     * Reinitialization clears previous-frame data, transform history, metrics
     * and the last error because state from another geometry cannot be reused.
     */
    bool initialize(uint32_t width, uint32_t height, const StabilizerParams& params);

    /**
     * @brief Process one BGRA video frame.
     * @param frame Non-empty CV_8UC4 frame matching the initialized dimensions.
     * @return Stabilized frame. Returns an empty matrix for invalid input or a
     *         fatal processing error. During tracking warm-up the input frame is
     *         returned unchanged; during tracking failures the last applied
     *         correction (decaying toward identity) keeps the output continuous.
     *
     * The returned matrix owns its storage independently of transient internal
     * buffers. Call get_last_error() after an empty result for diagnostics.
     */
    cv::Mat process_frame(const cv::Mat& frame);

    /**
     * @brief Replace runtime stabilization parameters.
     * @param params New validated parameter set.
     *
     * Settings take effect on the next processed frame. Geometry-dependent
     * state is retained; callers should use reset() when a clean trajectory is
     * required after a substantial configuration change.
     */
    void update_parameters(const StabilizerParams& params);

    /**
     * @brief Clear temporal stabilization state and cumulative metrics.
     *
     * The initialized geometry and current parameters remain available. The
     * next valid frame becomes the new tracking reference frame.
     */
    void reset();

    /**
     * @brief Return a snapshot of current cumulative performance metrics.
     * @return Metrics accumulated since initialize() or reset().
     */
    PerformanceMetrics get_performance_metrics() const;

    /**
     * @brief Access the current smoothed-motion input history.
     * @return Const reference to the internal transform deque.
     * @warning The reference is invalidated by non-const operations including
     *          process_frame(), reset(), initialize() and object destruction.
     */
    const std::deque<cv::Mat>& get_current_transforms() const;

    /**
     * @brief Check whether valid geometry has been initialized.
     * @return true when frames can be accepted for processing.
     */
    bool is_ready() const;

    /**
     * @brief Retrieve the most recent diagnostic error message.
     * @return Empty string when no error is recorded.
     */
    std::string get_last_error() const;

    /**
     * @brief Return a copy of the active stabilization parameters.
     * @return Current parameter set.
     */
    StabilizerParams get_current_params() const;

    /** @brief Create a low-latency preset for interactive game capture. */
    static StabilizerParams get_preset_gaming();

    /** @brief Create a balanced preset for live streaming. */
    static StabilizerParams get_preset_streaming();

    /** @brief Create a stronger smoothing preset for offline recording. */
    static StabilizerParams get_preset_recording();

    /**
     * @brief Validate frame type, dimensions and readiness for processing.
     * @param frame Candidate input frame.
     * @return true when frame can be passed to process_frame().
     */
    bool validate_frame(const cv::Mat& frame);

    /**
     * @brief Find the bounding rectangle of non-black image content.
     * @param frame Frame to inspect.
     * @return Bounding rectangle in frame coordinates, or an empty rectangle
     *         when no non-black content is present.
     *
     * This helper is used by crop and scale edge modes. Its cost is linear in
     * the number of pixels and should not be called redundantly per frame.
     */
    cv::Rect detect_content_bounds(const cv::Mat& frame);

private:
    bool detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points);
    bool track_features(const cv::Mat& prev_gray, const cv::Mat& curr_gray,
                       std::vector<cv::Point2f>& prev_pts, std::vector<cv::Point2f>& curr_pts,
                       float& success_rate);
    cv::Mat estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                              std::vector<cv::Point2f>& curr_pts,
                              float& inlier_ratio);
    cv::Mat make_failure_output(const cv::Mat& frame);
    cv::Mat smooth_transforms();
    cv::Mat smooth_transforms_kalman();
    cv::Mat apply_transform(const cv::Mat& frame, const cv::Mat& transform);

    inline cv::Mat smooth_transforms_optimized();
    inline void update_metrics(const std::chrono::high_resolution_clock::time_point& start_time);

    cv::Mat apply_edge_handling(const cv::Mat& frame, EdgeMode mode);

    static StabilizerParams create_preset(
        int smoothing_radius,
        float max_correction,
        int feature_count,
        float quality_level,
        float min_distance,
        EdgeMode edge_mode
    );

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool first_frame_ = true;

    StabilizerParams params_;

    cv::Mat prev_gray_;
    std::vector<cv::Point2f> prev_pts_;
    std::deque<cv::Mat> transforms_;
    cv::Mat trajectory_;
    cv::Mat last_correction_;
    int held_frames_ = 0;
    // Created lazily only when Kalman smoothing is selected so the default
    // moving-average path never constructs a cv::KalmanFilter instance.
    std::unique_ptr<KalmanTransformFilter> kalman_filter_;

    PerformanceMetrics metrics_;
    std::string last_error_;
    int consecutive_tracking_failures_ = 0;

    static constexpr int MIN_FEATURES_FOR_TRACKING = 4;
};

// Include OpenCV headers first to ensure all OpenCV types are defined
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/features2d.hpp>

#include "core/logging.hpp"
#include "core/stabilizer_core.hpp"
#include "core/stabilizer_constants.hpp"
#include "core/parameter_validation.hpp"
#include "core/frame_analyzer.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace StabilizerConstants;
using namespace StabilizerLogging;

namespace {

// Partial-affine 2x3 transform component helpers shared by the Kalman
// smoothing path and the motion-smoothing correction path.
cv::Vec4f transform_to_components(const cv::Mat& transform);
cv::Mat components_to_transform(const cv::Vec4f& components);

} // namespace

#define STAB_LOG_ERROR(...) CORE_LOG_ERROR(__VA_ARGS__)
#define STAB_LOG_WARNING(...) CORE_LOG_WARNING(__VA_ARGS__)
#define STAB_LOG_INFO(...) CORE_LOG_INFO(__VA_ARGS__)

bool StabilizerCore::initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params) {
    // Enable OpenCV SIMD optimizations for better performance
    // This enables platform-specific optimizations (SSE, AVX, NEON) without changing thread behavior
    // Note: This is separate from threading and is safe for OBS filter compatibility
    cv::setUseOptimized(true);

    // Set OpenCV to single-threaded mode to prevent internal threading issues
    // This is important for OBS filter compatibility and prevents potential crashes
    // when multiple StabilizerCore instances are created/destroyed rapidly
    cv::setNumThreads(1);

    // DESIGN NOTE: No mutex is used in StabilizerCore
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    // This design keeps the core algorithm simple and performant (KISS principle)

    // Validate dimensions before initialization
    // Zero or invalid dimensions cannot be processed and indicate configuration errors
    if (width == 0 || height == 0) {
        last_error_ = "Invalid dimensions: width and height must be greater than 0";
        CORE_LOG_ERROR("Cannot initialize with zero dimensions: %dx%d", width, height);
        return false;
    }

    // Validate minimum dimensions for feature detection
    // goodFeaturesToTrack requires sufficient image area to find corners
    if (width < MIN_IMAGE_SIZE || height < MIN_IMAGE_SIZE) {
        last_error_ = "Dimensions too small: minimum is " + std::to_string(MIN_IMAGE_SIZE) + "x" + std::to_string(MIN_IMAGE_SIZE);
        CORE_LOG_ERROR("Dimensions too small: %dx%d (minimum: %dx%d)", 
                      width, height, MIN_IMAGE_SIZE, MIN_IMAGE_SIZE);
        return false;
    }

    // Validate and clamp parameters using VALIDATION namespace
    // This ensures all parameters are within safe ranges and prevents DRY violations
    params_ = VALIDATION::validate_parameters(params);

    width_ = width;
    height_ = height;
    first_frame_ = true;
    prev_gray_ = cv::Mat();
    prev_pts_.clear();
    transforms_.clear();
    trajectory_ = cv::Mat::eye(3, 3, CV_64F);
    correction_smooth_ = cv::Mat();
    kalman_filter_.reset();
    metrics_ = {};
    consecutive_tracking_failures_ = 0;
    return true;
}

cv::Mat StabilizerCore::process_frame(const cv::Mat& frame) {
    auto start_time = std::chrono::high_resolution_clock::now();
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)

    try {
        // Early return for empty frames (likely common case)
        if (frame.empty()) {
            last_error_ = "Empty frame provided to StabilizerCore::process_frame";
            CORE_LOG_WARNING("Empty frame provided, skipping processing");
            return frame;
        }

        // Frame validation with branch prediction hints
        if (!validate_frame(frame)) {
            last_error_ = "Invalid frame dimensions: " + std::to_string(frame.rows) + "x" + std::to_string(frame.cols) + " in StabilizerCore::process_frame";
            CORE_LOG_ERROR("Invalid frame dimensions: %dx%d (expected: 32x32 to %dx%d)",
                          frame.rows, frame.cols, MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT);
            return cv::Mat();
        }

        // Early return for disabled stabilizer (common case)
        if (!params_.enabled) {
            CORE_LOG_DEBUG("Stabilizer disabled, returning original frame");
            return frame;
        }

    // Convert to grayscale using unified FRAME_UTILS to eliminate code duplication (DRY principle)
    // This consolidates color conversion logic that was duplicated in detect_content_bounds()
    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(frame);
    if (gray.empty()) {
        last_error_ = "Unsupported frame format in StabilizerCore::process_frame";
        CORE_LOG_ERROR("Failed to convert frame to grayscale (channels: %d)", frame.channels());
        return cv::Mat();
    }

    // Optical flow scales with pixel count, while the final warp must retain
    // the full source resolution. Track on a bounded working image, then map
    // the tracked points back before estimating motion in source pixels.
    // Keeping the working dimensions even avoids half-pixel asymmetry in
    // video formats.
    const double tracking_scale = std::min(
        1.0, static_cast<double>(OpticalFlow::MAX_TRACKING_DIMENSION) /
                 static_cast<double>(std::max(gray.cols, gray.rows)));
    cv::Mat tracking_gray;
    double tracking_scale_x = 1.0;
    double tracking_scale_y = 1.0;
    if (tracking_scale < 1.0) {
        const int tracking_width = std::max(
            2, static_cast<int>(std::lround(gray.cols * tracking_scale)) & ~1);
        const int tracking_height = std::max(
            2, static_cast<int>(std::lround(gray.rows * tracking_scale)) & ~1);
        cv::resize(gray, tracking_gray,
                   cv::Size(tracking_width, tracking_height),
                   0.0, 0.0, cv::INTER_AREA);
        tracking_scale_x = static_cast<double>(tracking_width) / gray.cols;
        tracking_scale_y = static_cast<double>(tracking_height) / gray.rows;
    } else {
        tracking_gray = gray;
    }

    if (first_frame_) {
        CORE_LOG_INFO("Processing first frame, initializing feature tracking");
        detect_features(tracking_gray, prev_pts_);
        if (prev_pts_.empty()) {
            CORE_LOG_WARNING("No features detected in first frame, using original frame");
            update_metrics(start_time);
            return frame;
        }
        prev_gray_ = tracking_gray.clone();
        first_frame_ = false;
        transforms_.push_back(cv::Mat::eye(2, 3, CV_64F));
        CORE_LOG_DEBUG("First frame processed, %zu features detected", prev_pts_.size());
        metrics_.successful_frames++;  // First frame is considered successful
        update_metrics(start_time);

        // Log first frame processing time separately (expected to be longer due to initialization)
        // The threshold is set to 2x the slow frame threshold since initialization overhead is expected
        double first_frame_time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count() * 1000.0;
        if (first_frame_time > Performance::SLOW_FRAME_THRESHOLD_MS * 2.0) {
            CORE_LOG_WARNING("First frame processing took %.2fms (expected overhead due to initialization)", first_frame_time);
        }

        return frame;
    }

    std::vector<cv::Point2f> curr_pts;
    float tracking_success_rate = 0.0f;
    if (!track_features(prev_gray_, tracking_gray, prev_pts_, curr_pts,
                        tracking_success_rate)) {
        consecutive_tracking_failures_++;
        metrics_.tracking_failures++;  // Track tracking failures for metrics
        CORE_LOG_WARNING("Feature tracking failed (attempt %d/5), success rate: %.2f",
                        consecutive_tracking_failures_, tracking_success_rate);
        if (consecutive_tracking_failures_ >= 5) {
            CORE_LOG_INFO("Tracking failed 5 times consecutively, re-detecting features");
            detect_features(tracking_gray, prev_pts_);
            // CRITICAL FIX: Update prev_gray_ to match the new features
            // Without this, there's a mismatch between feature points (from current frame)
            // and the previous grayscale image (from old frame), causing OpenCV pyramid errors
            prev_gray_ = tracking_gray.clone();
            consecutive_tracking_failures_ = 0;
        }
        update_metrics(start_time);
        return frame;
    }

    consecutive_tracking_failures_ = 0;

    std::vector<cv::Point2f> transform_prev_pts = prev_pts_;
    std::vector<cv::Point2f> transform_curr_pts = curr_pts;
    if (tracking_scale < 1.0) {
        // Estimate in source coordinates so RANSAC thresholds, translation
        // clamps, and rotation all retain their public full-resolution units.
        for (auto& point : transform_prev_pts) {
            point.x /= static_cast<float>(tracking_scale_x);
            point.y /= static_cast<float>(tracking_scale_y);
        }
        for (auto& point : transform_curr_pts) {
            point.x /= static_cast<float>(tracking_scale_x);
            point.y /= static_cast<float>(tracking_scale_y);
        }
    }

    cv::Mat transform = estimate_transform(transform_prev_pts,
                                           transform_curr_pts);
    if (transform.empty()) {
        CORE_LOG_WARNING("Transform estimation failed, returning original frame");
        update_metrics(start_time);
        return frame;
    }

    // Camera-trajectory correction. Accumulate the estimated motion, smooth
    // that trajectory over a finite history, and correct the current camera
    // position toward the smoothed position. Keeping both the current value
    // and history in the same fixed coordinate system is essential: rebasing
    // only trajectory_ would make the next sample incomparable with the
    // existing transforms_ entries and progressively weaken stabilization.
    cv::Mat incremental = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat transform_64;
    transform.convertTo(transform_64, CV_64F);
    transform_64.copyTo(incremental(cv::Rect(0, 0, 3, 2)));
    cv::Mat candidate_trajectory = incremental * trajectory_;

    cv::Mat inverse_trajectory;
    if (!cv::invert(candidate_trajectory, inverse_trajectory, cv::DECOMP_SVD)) {
        CORE_LOG_WARNING("Camera trajectory inversion failed, returning original frame");
        update_metrics(start_time);
        return frame;
    }

    cv::Mat current_trajectory = candidate_trajectory(cv::Rect(0, 0, 3, 2)).clone();
    transforms_.push_back(current_trajectory);
    while (transforms_.size() > static_cast<size_t>(params_.smoothing_radius)) {
        transforms_.pop_front();
    }

    cv::Mat smoothed_trajectory = smooth_transforms();
    cv::Mat smoothed_homogeneous = cv::Mat::eye(3, 3, CV_64F);
    smoothed_trajectory.copyTo(smoothed_homogeneous(cv::Rect(0, 0, 3, 2)));

    cv::Mat correction_homogeneous = smoothed_homogeneous * inverse_trajectory;
    cv::Mat correction = correction_homogeneous(cv::Rect(0, 0, 3, 2)).clone();

    // Keep the trajectory in its original camera coordinate system so every
    // entry in transforms_ remains comparable. The bounded correction below
    // prevents a bad track from producing an unbounded visible warp.
    trajectory_ = candidate_trajectory;

    // The correction is estimated from the previous frame, so applying it
    // directly leaves a one-frame-lag residual that is nearly as large as the
    // shake itself at high frequencies. Smooth the correction over time so
    // the per-frame warp does not oscillate; alpha = 0.6 attenuates the
    // high-frequency micro-jitter (a 30 fps EMA cutoff around 4.7 Hz) while
    // also suppressing the one-frame lag residual.
    if (correction_smooth_.empty()) {
        correction_smooth_ = correction.clone();
    } else {
        // Jump guard: a large per-frame correction change means the motion
        // estimate is unreliable (tracking failure during a fast pan or a
        // scene change). Limit how fast the correction may move so one bad
        // estimate cannot jerk the frame, then EMA the bounded correction.
        constexpr double kMaxCorrectionDeltaRatio = 0.02;
        constexpr double kMaxCorrectionDeltaAngleDeg = 0.75;
        constexpr double kMaxCorrectionDeltaScale = 0.02;
        cv::Vec4f bounded = transform_to_components(correction);
        const cv::Vec4f previous = transform_to_components(correction_smooth_);
        const double max_dx = kMaxCorrectionDeltaRatio * width_;
        const double max_dy = kMaxCorrectionDeltaRatio * height_;
        constexpr double max_angle = kMaxCorrectionDeltaAngleDeg * CV_PI / 180.0;
        constexpr double max_scale = kMaxCorrectionDeltaScale;
        bounded[0] = static_cast<float>(std::clamp(static_cast<double>(bounded[0]),
                                    previous[0] - max_dx, previous[0] + max_dx));
        bounded[1] = static_cast<float>(std::clamp(static_cast<double>(bounded[1]),
                                    previous[1] - max_dy, previous[1] + max_dy));
        bounded[2] = static_cast<float>(std::clamp(static_cast<double>(bounded[2]),
                                    previous[2] - max_angle, previous[2] + max_angle));
        bounded[3] = static_cast<float>(std::clamp(static_cast<double>(bounded[3]),
                                    previous[3] / (1.0 + max_scale),
                                    previous[3] * (1.0 + max_scale)));
        correction = components_to_transform(bounded);
        const double alpha = 0.6;
        correction_smooth_ = alpha * correction +
                             (1.0 - alpha) * correction_smooth_;
    }
    correction = correction_smooth_.clone();

    // Limit the correction so a bad motion estimate cannot jerk the frame.
    const double max_correction_ratio = params_.max_correction / 100.0;
    const double max_translation_x = max_correction_ratio * width_;
    const double max_translation_y = max_correction_ratio * height_;
    double *correction_ptr = correction.ptr<double>(0);
    correction_ptr[0] = std::clamp(correction_ptr[0],
                                   1.0 - max_correction_ratio,
                                   1.0 + max_correction_ratio);
    correction_ptr[1] = std::clamp(correction_ptr[1],
                                   -max_correction_ratio,
                                   max_correction_ratio);
    correction_ptr[2] = std::clamp(correction_ptr[2],
                                   -max_translation_x,
                                   max_translation_x);
    correction_ptr[3] = std::clamp(correction_ptr[3],
                                   -max_correction_ratio,
                                   max_correction_ratio);
    correction_ptr[4] = std::clamp(correction_ptr[4],
                                   1.0 - max_correction_ratio,
                                   1.0 + max_correction_ratio);
    correction_ptr[5] = std::clamp(correction_ptr[5],
                                   -max_translation_y,
                                   max_translation_y);

    tracking_gray.copyTo(prev_gray_);
    prev_pts_ = curr_pts;

    cv::Mat result = apply_transform(frame, correction);

    // Apply edge handling
    result = apply_edge_handling(result, params_.edge_mode);

    metrics_.successful_frames++;  // Track successful frame stabilization
    update_metrics(start_time);

    // Performance monitoring: Log slow frames to help identify performance bottlenecks
    // This threshold is set to 10ms (1/3 of 30fps requirement) to catch problematic frames
    // without overwhelming the log with normal processing times
    double processing_time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count() * 1000.0;
    if (processing_time > Performance::SLOW_FRAME_THRESHOLD_MS) {
        CORE_LOG_WARNING("Slow frame detected: %.2fms (features: %zu, resolution: %dx%d)",
                        processing_time, prev_pts_.size(), width_, height_);
    }

    return result;

    } catch (const cv::Exception& e) {
        last_error_ = std::string("OpenCV exception in process_frame: ") + e.what();
        log_opencv_exception("process_frame", e);
        return frame;
    } catch (const std::exception& e) {
        last_error_ = std::string("Standard exception in process_frame: ") + e.what();
        log_exception("process_frame", e);
        return frame;
    } catch (...) {
        last_error_ = "Unknown exception in process_frame";
        log_unknown_exception("process_frame");
        return frame;
    }
}

bool StabilizerCore::detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
    try {
        // Pre-allocate memory to avoid reallocations
        points.reserve(params_.feature_count);

        // min_distance is exposed in source-frame pixels. Preserve that
        // meaning when detection runs on the bounded optical-flow image.
        const double tracking_scale = std::min(
            static_cast<double>(gray.cols) / width_,
            static_cast<double>(gray.rows) / height_);
        const double tracking_min_distance = std::max(
            static_cast<double>(Distance::MIN),
            params_.min_distance * tracking_scale);

        // Standard OpenCV feature detection using Shi-Tomasi corner detection
        // This algorithm is well-suited for optical flow tracking and provides good
        // performance for real-time video stabilization without requiring custom NEON code
        cv::goodFeaturesToTrack(gray, points, params_.feature_count, params_.quality_level,
                               tracking_min_distance, cv::Mat(), params_.block_size,
                               params_.use_harris, params_.k);

        // Trim to actual count if fewer features found
        if (points.size() > params_.feature_count) {
            points.resize(params_.feature_count);
        }

        return !points.empty();

    } catch (const cv::Exception& e) {
        last_error_ = std::string("OpenCV exception in detect_features: ") + e.what();
        STAB_LOG_ERROR("OpenCV exception in detect_features: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        last_error_ = std::string("Standard exception in detect_features: ") + e.what();
        STAB_LOG_ERROR("Standard exception in detect_features: %s", e.what());
        return false;
    } catch (...) {
        last_error_ = "Unknown exception in detect_features";
        STAB_LOG_ERROR("Unknown exception in detect_features");
        return false;
    }
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

    try {
        // Lucas-Kanade optical flow window size (21x21)
        // This size provides good balance between tracking accuracy and performance
        // Based on empirical testing, values <15 lose tracking accuracy, values >25 reduce performance
        // Must be odd (requirement of cv::calcOpticalFlowPyrLK)
        static constexpr int LK_WINDOW_SIZE = 21;
        const cv::Size winSize(LK_WINDOW_SIZE, LK_WINDOW_SIZE);
        cv::TermCriteria termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS,
                                  OpticalFlow::MAX_ITERATIONS,
                                  OpticalFlow::CONVERGENCE_EPSILON);

        // Let Lucas-Kanade calculate the initial position. Passing a resized
        // vector filled with (0, 0) together with OPTFLOW_USE_INITIAL_FLOW
        // makes every track start at the top-left corner and causes repeated
        // tracking failures on real movement.
        cv::calcOpticalFlowPyrLK(prev_gray, curr_gray, prev_pts, curr_pts, status, err,
                                   winSize, 3, termcrit,  // Fixed pyramid levels: 3
                                   0);

        // Optimized filtering with branch prediction hints
        size_t i = 0;
        size_t tracked = 0;
        const size_t status_size = status.size();
        for (size_t j = 0; j < status_size; j++) {
            // Likely to be true, so we expect branch to be taken
            if (status[j]) {
                prev_pts[i] = prev_pts[j];
                curr_pts[i] = curr_pts[j];
                i++;
                tracked++;
            }
        }
        prev_pts.resize(i);
        curr_pts.resize(i);

        // Calculate success rate using original size (status_size) before resize
        // This is critical for correct feature refresh and adaptive stabilization
        // Using prev_pts.size() after resize would incorrectly show ~100% success even when tracking fails
        success_rate = status_size > 0 ? static_cast<float>(tracked) / static_cast<float>(status_size) : 0.0f;
        return i >= MIN_FEATURES_FOR_TRACKING;

    } catch (const cv::Exception& e) {
        last_error_ = std::string("OpenCV exception in track_features: ") + e.what();
        STAB_LOG_ERROR("OpenCV exception in track_features: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        last_error_ = std::string("Standard exception in track_features: ") + e.what();
        STAB_LOG_ERROR("Standard exception in track_features: %s", e.what());
        return false;
    } catch (...) {
        last_error_ = "Unknown exception in track_features";
        STAB_LOG_ERROR("Unknown exception in track_features");
        return false;
    }
}

cv::Mat StabilizerCore::estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                                              std::vector<cv::Point2f>& curr_pts) {
    try {
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
        const double max_correction_ratio = params_.max_correction / 100.0;
        const double max_translation_x = max_correction_ratio * width_;
        const double max_translation_y = max_correction_ratio * height_;
        double* ptr = transform.ptr<double>(0);

        // Limit rotation and translation components
        // 2x3 transform matrix indices are defined in smooth_transforms_optimized
        // Using named constants to avoid magic numbers and improve readability
        constexpr int TX_00 = 0; // a00: scale x
        constexpr int TX_01 = 1; // a01: shear x
        constexpr int TX_02 = 2; // a02: translation x
        constexpr int TX_10 = 3; // a10: shear y
        constexpr int TX_11 = 4; // a11: scale y
        constexpr int TX_12 = 5; // a12: translation y
        ptr[TX_00] = std::clamp(ptr[TX_00], 1.0 - max_correction_ratio, 1.0 + max_correction_ratio);
        ptr[TX_01] = std::clamp(ptr[TX_01], -max_correction_ratio, max_correction_ratio);
        ptr[TX_02] = std::clamp(ptr[TX_02], -max_translation_x, max_translation_x);
        ptr[TX_10] = std::clamp(ptr[TX_10], -max_correction_ratio, max_correction_ratio);
        ptr[TX_11] = std::clamp(ptr[TX_11], 1.0 - max_correction_ratio, 1.0 + max_correction_ratio);
        ptr[TX_12] = std::clamp(ptr[TX_12], -max_translation_y, max_translation_y);

        return transform;

    } catch (const cv::Exception& e) {
        last_error_ = std::string("OpenCV exception in estimate_transform: ") + e.what();
        STAB_LOG_ERROR("OpenCV exception in estimate_transform: %s", e.what());
        return cv::Mat::eye(2, 3, CV_64F);
    } catch (const std::exception& e) {
        last_error_ = std::string("Standard exception in estimate_transform: ") + e.what();
        STAB_LOG_ERROR("Standard exception in estimate_transform: %s", e.what());
        return cv::Mat::eye(2, 3, CV_64F);
    } catch (...) {
        last_error_ = "Unknown exception in estimate_transform";
        STAB_LOG_ERROR("Unknown exception in estimate_transform");
        return cv::Mat::eye(2, 3, CV_64F);
    }
}

cv::Mat StabilizerCore::smooth_transforms() {
    if (params_.smoothing_mode == SmoothingMode::Kalman) {
        return smooth_transforms_kalman();
    }
    return smooth_transforms_optimized();
}

namespace {

// Convert a partial-affine 2x3 transform to [dx, dy, angle, scale].
cv::Vec4f transform_to_components(const cv::Mat& transform) {
    const double* ptr = transform.ptr<double>(0);
    constexpr int TX_00 = 0;
    constexpr int TX_02 = 2;
    constexpr int TX_10 = 3;
    constexpr int TX_12 = 5;
    const double scale = std::sqrt(ptr[TX_00] * ptr[TX_00] + ptr[TX_10] * ptr[TX_10]);
    const double angle = std::atan2(ptr[TX_10], ptr[TX_00]);
    return {
        static_cast<float>(ptr[TX_02]),
        static_cast<float>(ptr[TX_12]),
        static_cast<float>(angle),
        static_cast<float>(scale),
    };
}

// Rebuild a partial-affine 2x3 transform from [dx, dy, angle, scale].
cv::Mat components_to_transform(const cv::Vec4f& components) {
    const float dx = components[0];
    const float dy = components[1];
    const float angle = components[2];
    const float scale = components[3];
    const double cos_a = std::cos(angle);
    const double sin_a = std::sin(angle);
    cv::Mat transform = cv::Mat::eye(2, 3, CV_64F);
    double* ptr = transform.ptr<double>(0);
    ptr[0] = scale * cos_a;
    ptr[1] = -scale * sin_a;
    ptr[2] = dx;
    ptr[3] = scale * sin_a;
    ptr[4] = scale * cos_a;
    ptr[5] = dy;
    return transform;
}

} // namespace

cv::Mat StabilizerCore::smooth_transforms_kalman() {
    if (transforms_.empty()) {
        return cv::Mat::eye(2, 3, CV_64F);
    }

    if (!kalman_filter_) {
        kalman_filter_ = std::make_unique<KalmanTransformFilter>();
    }

    // The Kalman filter keeps a constant-velocity estimate of the correction
    // components. Feeding it the raw per-frame transform smooths jitter while
    // still following sustained camera motion.
    const cv::Vec4f measurement = transform_to_components(transforms_.back());
    const cv::Vec4f corrected = kalman_filter_->update(measurement, 1.0f);
    return components_to_transform(corrected);
}

cv::Mat StabilizerCore::smooth_transforms_optimized() {
    if (transforms_.empty()) {
        return cv::Mat::eye(2, 3, CV_64F);
    }

    const size_t size = transforms_.size();
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);

    // Standard transform averaging without NEON-specific optimizations
    // This implementation provides good performance for real-time video stabilization
    // and avoids complexity from platform-specific code that isn't currently needed
    //
    // 2x3 transform matrix indices (named constants for readability)
    constexpr int TX_00 = 0;  // a00: scale x
    constexpr int TX_01 = 1;  // a01: shear x
    constexpr int TX_02 = 2;  // a02: translation x
    constexpr int TX_10 = 3;  // a10: shear y
    constexpr int TX_11 = 4;  // a11: scale y
    constexpr int TX_12 = 5;  // a12: translation y

    auto* ptr = smoothed.ptr<double>(0);
    const double inv_size = 1.0 / static_cast<double>(size);

    for (const auto& t : transforms_) {
        const double* t_ptr = t.ptr<double>(0);
        ptr[TX_00] += t_ptr[TX_00]; ptr[TX_01] += t_ptr[TX_01]; ptr[TX_02] += t_ptr[TX_02];
        ptr[TX_10] += t_ptr[TX_10]; ptr[TX_11] += t_ptr[TX_11]; ptr[TX_12] += t_ptr[TX_12];
    }

    ptr[TX_00] *= inv_size; ptr[TX_01] *= inv_size; ptr[TX_02] *= inv_size;
    ptr[TX_10] *= inv_size; ptr[TX_11] *= inv_size; ptr[TX_12] *= inv_size;

    return smoothed;
}

inline void StabilizerCore::update_metrics(const std::chrono::high_resolution_clock::time_point& start_time) {
    auto end_time = std::chrono::high_resolution_clock::now();
    double processing_time = std::chrono::duration<double>(end_time - start_time).count();
    metrics_.total_frames++;
    metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.total_frames - 1) + processing_time) / metrics_.total_frames;
}

cv::Mat StabilizerCore::apply_transform(const cv::Mat& frame, const cv::Mat& transform) {
    try {
        cv::Mat warped_frame;
        // Cubic interpolation keeps edges sharp in the small sources used by
        // the README examples. At HD resolutions its single-threaded cost can
        // exceed a frame budget, while a source pixel is already visually
        // smaller, so use linear interpolation above VGA pixel count.
        constexpr int kCubicInterpolationMaxPixels = 640 * 480;
        const int interpolation = frame.total() <= kCubicInterpolationMaxPixels
                                      ? cv::INTER_CUBIC
                                      : cv::INTER_LINEAR;
        cv::warpAffine(frame, warped_frame, transform, frame.size(),
                       interpolation);
        return warped_frame;
    } catch (const cv::Exception& e) {
        last_error_ = std::string("OpenCV exception in apply_transform: ") + e.what();
        STAB_LOG_ERROR("OpenCV exception in apply_transform: %s", e.what());
        return frame.clone();
    } catch (const std::exception& e) {
        last_error_ = std::string("Standard exception in apply_transform: ") + e.what();
        STAB_LOG_ERROR("Standard exception in apply_transform: %s", e.what());
        return frame.clone();
    } catch (...) {
        last_error_ = "Unknown exception in apply_transform";
        STAB_LOG_ERROR("Unknown exception in apply_transform");
        return frame.clone();
    }
}

cv::Rect StabilizerCore::detect_content_bounds(const cv::Mat& frame) {
    // Delegate content-bound detection to the reusable FrameAnalyzer
    // (Issue #313). Keep the legacy contract: an empty result from the
    // analyzer means no detectable content, so fall back to the full frame.
    const cv::Rect bounds = FrameAnalyzer::detect_content_bounds(frame);
    if (bounds.width == 0 || bounds.height == 0) {
        return cv::Rect(0, 0, frame.cols, frame.rows);
    }
    return bounds;
}

cv::Mat StabilizerCore::apply_edge_handling(const cv::Mat& frame, EdgeMode mode) {
    try {
        switch (mode) {
            case EdgeMode::Padding:
                // Padding mode: Return frame as-is with black borders
                return frame;

            case EdgeMode::Crop: {
                // Crop mode: Remove black borders from edges
                cv::Rect bounds = detect_content_bounds(frame);

                // Ensure crop region is valid
                if (bounds.width <= 0 || bounds.height <= 0) {
                    return frame;
                }

                // Crop the frame with comprehensive bounds checking
                // Clamping ensures ROI coordinates are always within valid range
                // This prevents OpenCV exceptions when creating cv::Mat from ROI
                int roi_x = std::max(0, bounds.x);
                int roi_y = std::max(0, bounds.y);
                int roi_width = std::min(bounds.width, frame.cols - roi_x);
                int roi_height = std::min(bounds.height, frame.rows - roi_y);

                // OBS async frames use NV12/I420 output, whose chroma planes
                // require even dimensions. Round the crop down to an even size
                // so downstream color conversion never asserts on odd frames.
                roi_width &= ~1;
                roi_height &= ~1;

                // Only crop if we have a valid ROI (positive dimensions)
                if (roi_width > 0 && roi_height > 0) {
                    cv::Rect clamped_bounds(roi_x, roi_y, roi_width, roi_height);
                    return frame(clamped_bounds).clone();
                }
                return frame;
            }

            case EdgeMode::Scale: {
                // Scale mode: Scale frame to fill original dimensions
                cv::Rect bounds = detect_content_bounds(frame);

                // Ensure crop region is valid
                if (bounds.width <= 0 || bounds.height <= 0) {
                    return frame;
                }

                // Calculate scale factor to fill original frame
                // Ensure scale is >= 1.0 to prevent upscaling content beyond original frame size
                double scale_x = static_cast<double>(frame.cols) / bounds.width;
                double scale_y = static_cast<double>(frame.rows) / bounds.height;
                double scale = std::max(1.0, std::min(scale_x, scale_y));

                // Scale the frame
                cv::Mat scaled;
                cv::resize(frame, scaled, cv::Size(), scale, scale, cv::INTER_LINEAR);

                // Center the scaled frame with bounds checking
                cv::Mat result(frame.size(), frame.type(), cv::Scalar(0, 0, 0, 255));
                int offset_x = (frame.cols - scaled.cols) / 2;
                int offset_y = (frame.rows - scaled.rows) / 2;

                // Ensure offset is always non-negative to simplify ROI calculations
                // This prevents the issue where src_x = roi_x - offset_x could be negative
                offset_x = std::max(0, offset_x);
                offset_y = std::max(0, offset_y);

                // Calculate ROI in destination (result) with bounds checking
                int dst_x = offset_x;
                int dst_y = offset_y;
                int dst_width = std::min(scaled.cols, frame.cols - dst_x);
                int dst_height = std::min(scaled.rows, frame.rows - dst_y);

                // Only copy if we have a valid ROI
                if (dst_width > 0 && dst_height > 0) {
                    cv::Rect dst_roi(dst_x, dst_y, dst_width, dst_height);

                    // Calculate corresponding ROI in source (scaled)
                    // With non-negative offset, source ROI always starts at (0, 0)
                    cv::Rect src_roi(0, 0, dst_width, dst_height);

                    // Validate source ROI bounds before copying
                    if (src_roi.x >= 0 && src_roi.y >= 0 &&
                        src_roi.x + src_roi.width <= scaled.cols &&
                        src_roi.y + src_roi.height <= scaled.rows) {
                        scaled(src_roi).copyTo(result(dst_roi));
                    }
                }

                return result;
            }

            default:
                return frame;
        }
    } catch (const cv::Exception& e) {
        last_error_ = std::string("OpenCV exception in apply_edge_handling: ") + e.what();
        STAB_LOG_ERROR("OpenCV exception in apply_edge_handling: %s", e.what());
        return frame.clone();
    } catch (const std::exception& e) {
        last_error_ = std::string("Standard exception in apply_edge_handling: ") + e.what();
        STAB_LOG_ERROR("Standard exception in apply_edge_handling: %s", e.what());
        return frame.clone();
    } catch (...) {
        last_error_ = "Unknown exception in apply_edge_handling";
        STAB_LOG_ERROR("Unknown exception in apply_edge_handling");
        return frame.clone();
    }
}

void StabilizerCore::update_parameters(const StabilizerCore::StabilizerParams& params) {
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    params_ = params;
}

void StabilizerCore::reset() {
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    first_frame_ = true;
    prev_gray_.release();
    prev_pts_.clear();
    transforms_.clear();
    trajectory_ = cv::Mat::eye(3, 3, CV_64F);
    correction_smooth_ = cv::Mat();
    metrics_ = {};
    consecutive_tracking_failures_ = 0;
}

StabilizerCore::PerformanceMetrics StabilizerCore::get_performance_metrics() const {
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    return metrics_;
}

const std::deque<cv::Mat>& StabilizerCore::get_current_transforms() const {
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    return transforms_;
}

bool StabilizerCore::is_ready() const {
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    return width_ > 0 && height_ > 0;
}

std::string StabilizerCore::get_last_error() const {
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    return last_error_;
}

StabilizerCore::StabilizerParams StabilizerCore::get_current_params() const {
    // DESIGN NOTE: No mutex is used in StabilizerCore (single-threaded design)
    // Thread safety is provided by StabilizerWrapper layer (caller's responsibility)
    return params_;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_gaming() {
    return create_preset(
        Smoothing::GAMING_RADIUS,
        Correction::GAMING_MAX,
        Features::GAMING_COUNT,
        Quality::GAMING_LEVEL,
        Distance::GAMING,
        EdgeMode::Padding  // Performance
    );
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_streaming() {
    return create_preset(
        Smoothing::STREAMING_RADIUS,
        Correction::STREAMING_MAX,
        Features::DEFAULT_COUNT,
        Quality::DEFAULT_LEVEL,
        Distance::DEFAULT,
        EdgeMode::Crop  // Quality
    );
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_recording() {
    return create_preset(
        Smoothing::RECORDING_RADIUS,
        Correction::RECORDING_MAX,
        Features::RECORDING_COUNT,
        Quality::RECORDING_LEVEL,
        Distance::RECORDING,
        EdgeMode::Scale  // Full frame coverage
    );
}

// Helper function to reduce code duplication in preset functions (DRY principle)
StabilizerCore::StabilizerParams StabilizerCore::create_preset(
    int smoothing_radius,
    float max_correction,
    int feature_count,
    float quality_level,
    float min_distance,
    EdgeMode edge_mode
) {
    StabilizerParams params;
    params.smoothing_radius = smoothing_radius;
    params.max_correction = max_correction;
    params.feature_count = feature_count;
    params.quality_level = quality_level;
    params.min_distance = min_distance;
    params.block_size = Block::DEFAULT_SIZE;
    params.use_harris = false;
    params.k = Harris::DEFAULT_K;
    params.enabled = true;
    params.edge_mode = edge_mode;
    return params;
}

bool StabilizerCore::validate_frame(const cv::Mat& frame) {
    // Use common validation from FRAME_UTILS to eliminate code duplication (DRY principle)
    // The common validation checks: empty, dimensions (rows/cols > 0), depth (CV_8U), channels (1, 3, 4)
    if (!FRAME_UTILS::Validation::validate_cv_mat(frame)) {
        return false;
    }

    // Add MIN/MAX size checks specific to StabilizerCore
    // These checks are specific to the stabilization algorithm's requirements
    if (frame.rows < MIN_IMAGE_SIZE || frame.cols < MIN_IMAGE_SIZE) {
        return false;
    }
    if (frame.rows > MAX_IMAGE_HEIGHT || frame.cols > MAX_IMAGE_WIDTH) {
        return false;
    }

    return true;
}

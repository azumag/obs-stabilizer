// Include OpenCV headers first to ensure all OpenCV types are defined
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/features2d.hpp>

#include "core/logging.hpp"
#include "core/stabilizer_core.hpp"
#include "core/stabilizer_constants.hpp"
#include "core/parameter_validation.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace StabilizerConstants;
using namespace StabilizerLogging;

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

    if (first_frame_) {
        CORE_LOG_INFO("Processing first frame, initializing feature tracking");
        detect_features(gray, prev_pts_);
        if (prev_pts_.empty()) {
            CORE_LOG_WARNING("No features detected in first frame, using original frame");
            update_metrics(start_time);
            return frame;
        }
        prev_gray_ = gray.clone();
        first_frame_ = false;
        transforms_.push_back(cv::Mat::eye(2, 3, CV_64F));
        CORE_LOG_DEBUG("First frame processed, %zu features detected", prev_pts_.size());
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
    // Pre-resize curr_pts to match prev_pts_ size - required by cv::calcOpticalFlowPyrLK()
    // The function expects nextPts (curr_pts) to have the same size as prevPts (prev_pts_)
    // Even though calcOpticalFlowPyrLK writes to curr_pts, it still needs proper sizing
    curr_pts.resize(prev_pts_.size());
    float tracking_success_rate = 0.0f;
    if (!track_features(prev_gray_, gray, prev_pts_, curr_pts, tracking_success_rate)) {
        consecutive_tracking_failures_++;
        CORE_LOG_WARNING("Feature tracking failed (attempt %d/5), success rate: %.2f",
                       consecutive_tracking_failures_, tracking_success_rate);
        if (consecutive_tracking_failures_ >= 5) {
            CORE_LOG_INFO("Tracking failed 5 times consecutively, re-detecting features");
            detect_features(gray, prev_pts_);
            // CRITICAL FIX: Update prev_gray_ to match the new features
            // Without this, there's a mismatch between feature points (from current frame)
            // and the previous grayscale image (from old frame), causing OpenCV pyramid errors
            prev_gray_ = gray.clone();
            consecutive_tracking_failures_ = 0;
        }
        update_metrics(start_time);
        return frame;
    }

    consecutive_tracking_failures_ = 0;

    cv::Mat transform = estimate_transform(prev_pts_, curr_pts);
    if (transform.empty()) {
        CORE_LOG_WARNING("Transform estimation failed, returning original frame");
        update_metrics(start_time);
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

    // Apply edge handling
    result = apply_edge_handling(result, params_.edge_mode);

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

        // Standard OpenCV feature detection using Shi-Tomasi corner detection
        // This algorithm is well-suited for optical flow tracking and provides good
        // performance for real-time video stabilization without requiring custom NEON code
        cv::goodFeaturesToTrack(gray, points, params_.feature_count, params_.quality_level,
                               params_.min_distance, cv::Mat(), params_.block_size,
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
        cv::TermCriteria termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, OpticalFlow::MAX_ITERATIONS, OpticalFlow::EPSILON);

        // Pre-resize curr_pts to match prev_pts size - required by cv::calcOpticalFlowPyrLK()
        // The function expects nextPts (curr_pts) to have the same size as prevPts (prev_pts)
        // Even though calcOpticalFlowPyrLK writes to curr_pts, it still needs proper sizing
        curr_pts.resize(prev_pts.size());

        cv::calcOpticalFlowPyrLK(prev_gray, curr_gray, prev_pts, curr_pts, status, err,
                                   winSize, 3, termcrit,  // Fixed pyramid levels: 3
                                   cv::OPTFLOW_USE_INITIAL_FLOW);

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
        const double max_correction = params_.max_correction / 100.0;
        double* ptr = transform.ptr<double>(0);

        // Limit rotation and translation components
        ptr[0] = std::clamp(ptr[0], 1.0 - max_correction, 1.0 + max_correction);
        ptr[1] = std::clamp(ptr[1], -max_correction, max_correction);
        ptr[2] = std::clamp(ptr[2], -max_correction, max_correction);
        ptr[3] = std::clamp(ptr[3], -max_correction, max_correction);
        ptr[4] = std::clamp(ptr[4], 1.0 - max_correction, 1.0 + max_correction);

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
    return smooth_transforms_optimized();
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
    auto* ptr = smoothed.ptr<double>(0);
    const double inv_size = 1.0 / static_cast<double>(size);

    for (const auto& t : transforms_) {
        const double* t_ptr = t.ptr<double>(0);
        ptr[0] += t_ptr[0]; ptr[1] += t_ptr[1]; ptr[2] += t_ptr[2];
        ptr[3] += t_ptr[3]; ptr[4] += t_ptr[4]; ptr[5] += t_ptr[5];
    }

    ptr[0] *= inv_size; ptr[1] *= inv_size; ptr[2] *= inv_size;
    ptr[3] *= inv_size; ptr[4] *= inv_size; ptr[5] *= inv_size;

    return smoothed;
}

inline void StabilizerCore::update_metrics(const std::chrono::high_resolution_clock::time_point& start_time) {
    auto end_time = std::chrono::high_resolution_clock::now();
    double processing_time = std::chrono::duration<double>(end_time - start_time).count();
    metrics_.frame_count++;
    metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
}

cv::Mat StabilizerCore::apply_transform(const cv::Mat& frame, const cv::Mat& transform) {
    try {
        cv::Mat warped_frame;
        cv::warpAffine(frame, warped_frame, transform, frame.size());
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
    // Convert to grayscale using unified FRAME_UTILS to eliminate code duplication (DRY principle)
    // This function is called for both process_frame() and detect_content_bounds(), so centralizing it
    // avoids maintaining duplicate color conversion logic
    cv::Mat gray = FRAME_UTILS::ColorConversion::convert_to_grayscale(frame);
    if (gray.empty()) {
        return cv::Rect(0, 0, frame.cols, frame.rows);
    }

    // Use OpenCV's findNonZero() for efficient content detection
    // This is O(n) where n is the number of pixels, but heavily optimized in OpenCV
    // Previous implementation was O(width * height * 4) with 4 separate scan loops
    // The new approach uses vectorized operations and is much faster for typical video frames
    cv::Mat binary;
    cv::threshold(gray, binary, ContentDetection::CONTENT_THRESHOLD, 255, cv::THRESH_BINARY);

    std::vector<cv::Point> non_zero;
    cv::findNonZero(binary, non_zero);

    // If no content detected (e.g., all-black frame), return full frame
    if (non_zero.empty()) {
        return cv::Rect(0, 0, frame.cols, frame.rows);
    }

    // boundingRect() efficiently computes the minimal rectangle containing all non-zero pixels
    // This is a single O(n) pass through the non-zero pixel list
    cv::Rect bounds = cv::boundingRect(non_zero);
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
                double scale_x = static_cast<double>(frame.cols) / bounds.width;
                double scale_y = static_cast<double>(frame.rows) / bounds.height;
                double scale = std::min(scale_x, scale_y);

                // Scale the frame
                cv::Mat scaled;
                cv::resize(frame, scaled, cv::Size(), scale, scale, cv::INTER_LINEAR);

                // Center the scaled frame with bounds checking
                cv::Mat result(frame.size(), frame.type(), cv::Scalar(0, 0, 0, 255));
                int offset_x = (frame.cols - scaled.cols) / 2;
                int offset_y = (frame.rows - scaled.rows) / 2;

                // Ensure ROI coordinates are within bounds
                // This prevents OpenCV exceptions when scaled.cols > frame.cols or scaled.rows > frame.rows
                int roi_x = std::max(0, offset_x);
                int roi_y = std::max(0, offset_y);
                int roi_width = std::min(scaled.cols, frame.cols - roi_x);
                int roi_height = std::min(scaled.rows, frame.rows - roi_y);

                // Only copy if we have a valid ROI
                if (roi_width > 0 && roi_height > 0) {
                    cv::Rect roi(roi_x, roi_y, roi_width, roi_height);

                    // Calculate corresponding ROI in the scaled frame
                    int src_x = roi_x - offset_x;
                    int src_y = roi_y - offset_y;
                    cv::Rect src_roi(src_x, src_y, roi_width, roi_height);

                    // Copy the valid region
                    scaled(src_roi).copyTo(result(roi));
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
    prev_gray_ = cv::Mat::zeros(height_, width_, CV_8UC1);
    prev_pts_.clear();
    transforms_.clear();
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
    StabilizerParams params;
    params.smoothing_radius = Smoothing::GAMING_RADIUS;
    params.max_correction = Correction::GAMING_MAX;
    params.feature_count = Features::GAMING_COUNT;
    params.quality_level = Quality::GAMING_LEVEL;
    params.min_distance = Distance::GAMING;
    params.block_size = Block::DEFAULT_SIZE;
    params.use_harris = false;
    params.k = Harris::DEFAULT_K;
    params.enabled = true;
    params.edge_mode = EdgeMode::Padding; // Performance
    return params;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_streaming() {
    StabilizerParams params;
    params.smoothing_radius = Smoothing::STREAMING_RADIUS;
    params.max_correction = Correction::STREAMING_MAX;
    params.feature_count = Features::DEFAULT_COUNT;
    params.quality_level = Quality::DEFAULT_LEVEL;
    params.min_distance = Distance::DEFAULT;
    params.block_size = Block::DEFAULT_SIZE;
    params.use_harris = false;
    params.k = Harris::DEFAULT_K;
    params.enabled = true;
    params.edge_mode = EdgeMode::Crop; // Quality
    return params;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_recording() {
    StabilizerParams params;
    params.smoothing_radius = Smoothing::RECORDING_RADIUS;
    params.max_correction = Correction::RECORDING_MAX;
    params.feature_count = Features::RECORDING_COUNT;
    params.quality_level = Quality::RECORDING_LEVEL;
    params.min_distance = Distance::RECORDING;
    params.block_size = Block::DEFAULT_SIZE;
    params.use_harris = false;
    params.k = Harris::DEFAULT_K;
    params.enabled = true;
    params.edge_mode = EdgeMode::Scale; // Full frame coverage
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

    // Validate pixel depth - only 8-bit unsigned formats are supported
    // 16-bit (CV_16UC*) and other formats require different processing pipelines
    // and are not compatible with the current stabilization algorithms
    int depth = frame.depth();
    if (depth != CV_8U) {
        // Unsupported bit depth - only 8-bit unsigned is supported
        return false;
    }

    // Validate channel count
    // 1-channel (grayscale), 3-channel (BGR), and 4-channel (BGRA) formats are supported
    // 2-channel formats are not supported by the current processing pipeline
    int channels = frame.channels();
    if (channels != 1 && channels != 3 && channels != 4) {
        // Unsupported channel count
        return false;
    }

    return true;
}

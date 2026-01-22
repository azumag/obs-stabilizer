#include "core/stabilizer_core.hpp"
#include "core/stabilizer_constants.hpp"
#include "core/neon_feature_detection.hpp"
#include "core/logging.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace StabilizerConstants;

#define STAB_LOG_ERROR(...) CORE_LOG_ERROR(__VA_ARGS__)
#define STAB_LOG_WARNING(...) CORE_LOG_WARNING(__VA_ARGS__)
#define STAB_LOG_INFO(...) CORE_LOG_INFO(__VA_ARGS__)

// (existing implementation)
bool StabilizerCore::initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate parameters before initialization
    if (!validate_parameters(params)) {
        last_error_ = "Invalid parameters provided to initialize";
        return false;
    }
    
    width_ = width;
    height_ = height;
    params_ = params;
    first_frame_ = true;
    prev_gray_ = cv::Mat(height, width, CV_8UC1);
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
    metrics_ = {};
    return true;
}

cv::Mat StabilizerCore::process_frame(const cv::Mat& frame) {
    auto start_time = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        // Early return for empty frames (likely common case)
        if (frame.empty()) {
            last_error_ = "Empty frame provided";
            return frame;
        }

        // Frame validation with branch prediction hints
        if (!validate_frame(frame)) {
            last_error_ = "Invalid frame dimensions";
            return cv::Mat();
        }

        // Early return for disabled stabilizer (common case)
        if (!params_.enabled) {
            return frame;
        }

    // Optimized color conversion with branch prediction hints
    cv::Mat gray;
    const int num_channels = frame.channels();

    // Standard OpenCV color conversion
    if (num_channels == 4) {
        cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
    } else if (num_channels == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else if (num_channels == 1) {
        gray = frame;
    } else {
        last_error_ = "Unsupported frame format";
        return cv::Mat();
    }

    if (first_frame_) {
        detect_features(gray, prev_pts_);
        if (prev_pts_.empty()) {
            update_metrics(start_time);
            return frame;
        }
        prev_gray_ = gray.clone();
        first_frame_ = false;
        transforms_.push_back(cv::Mat::eye(2, 3, CV_64F));
        update_metrics(start_time);
        return frame;
    }

    std::vector<cv::Point2f> curr_pts;
    curr_pts.resize(prev_pts_.size());
    float tracking_success_rate = 0.0f;
    if (!track_features(prev_gray_, gray, prev_pts_, curr_pts, tracking_success_rate)) {
        consecutive_tracking_failures_++;
        if (consecutive_tracking_failures_ >= 5) {
            detect_features(gray, prev_pts_);
            consecutive_tracking_failures_ = 0;
            frames_since_last_refresh_ = 0;
        }
        update_metrics(start_time);
        return frame;
    }

    consecutive_tracking_failures_ = 0;
    frames_since_last_refresh_++;

    // Use optimized feature refresh logic
    if (should_refresh_features(tracking_success_rate, frames_since_last_refresh_)) {
        // Use lookup table for adaptive feature count selection
        static const float success_thresholds[] = {0.3f, 0.7f};
        static const int feature_counts[] = {
            params_.adaptive_feature_max,
            params_.feature_count,
            params_.adaptive_feature_min
        };
        
        int adaptive_count = params_.feature_count;
        if (tracking_success_rate < success_thresholds[0]) {
            adaptive_count = feature_counts[0];
        } else if (tracking_success_rate > success_thresholds[1]) {
            adaptive_count = feature_counts[2];
        }

        std::vector<cv::Point2f> new_features;
        cv::goodFeaturesToTrack(gray, new_features, adaptive_count,
                              params_.quality_level, params_.min_distance,
                              cv::Mat(), params_.block_size,
                              params_.use_harris, params_.k);

        if (!new_features.empty() && new_features.size() >= MIN_FEATURES_FOR_TRACKING) {
            prev_pts_ = new_features;
            frames_since_last_refresh_ = 0;
        }
    }

    cv::Mat transform = estimate_transform(prev_pts_, curr_pts);
    if (transform.empty()) {
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

    // Apply edge handling (Issue #226)
    result = apply_edge_handling(result, params_.edge_mode);

    update_metrics(start_time);

    return result;

    } catch (const cv::Exception& e) {
        last_error_ = std::string("OpenCV exception in process_frame: ") + e.what();
        STAB_LOG_ERROR("OpenCV exception in process_frame: %s", e.what());
        return frame;
    } catch (const std::exception& e) {
        last_error_ = std::string("Standard exception in process_frame: ") + e.what();
        STAB_LOG_ERROR("Standard exception in process_frame: %s", e.what());
        return frame;
    } catch (...) {
        last_error_ = "Unknown exception in process_frame";
        STAB_LOG_ERROR("Unknown exception in process_frame");
        return frame;
    }
}

bool StabilizerCore::detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
    try {
        // Pre-allocate memory to avoid reallocations
        points.reserve(params_.feature_count);

        #ifndef BUILD_STANDALONE
        // Use platform-optimized feature detection when available
        if (PlatformOptimization::is_arm64() && gray.isContinuous() &&
            gray.channels() == 1 && gray.depth() == CV_8U) {

            static AppleOptimization::NEONFeatureDetector neon_detector;
            neon_detector.set_quality_level(params_.quality_level);
            neon_detector.set_min_distance(params_.min_distance);
            neon_detector.set_block_size(params_.block_size);
            neon_detector.set_ksize(params_.k);
            neon_detector.detect_features(gray, points);
        } else {
        #endif
            // Standard OpenCV feature detection
            cv::goodFeaturesToTrack(gray, points, params_.feature_count, params_.quality_level,
                                   params_.min_distance, cv::Mat(), params_.block_size,
                                   params_.use_harris, params_.k);
        #ifndef BUILD_STANDALONE
        }
        #endif

        // Trim to actual count if fewer features found
        if (points.size() > params_.feature_count) {
            points.resize(params_.feature_count);
        }

        return !points.empty();

    } catch (const cv::Exception& e) {
        STAB_LOG_ERROR("OpenCV exception in detect_features: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        STAB_LOG_ERROR("Standard exception in detect_features: %s", e.what());
        return false;
    } catch (...) {
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
        cv::Size winSize(params_.optical_flow_window_size, params_.optical_flow_window_size);
        cv::TermCriteria termcrit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, OpticalFlow::MAX_ITERATIONS, OpticalFlow::EPSILON);

        // Use pre-allocated vectors to avoid reallocations
        cv::calcOpticalFlowPyrLK(prev_gray, curr_gray, prev_pts, curr_pts, status, err,
                                   winSize, params_.optical_flow_pyramid_levels, termcrit,
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

        // Use multiplication instead of division for better performance
        const float inv_size = 1.0f / static_cast<float>(prev_pts.size());
        success_rate = static_cast<float>(tracked) * inv_size;
        return i >= MIN_FEATURES_FOR_TRACKING;

    } catch (const cv::Exception& e) {
        STAB_LOG_ERROR("OpenCV exception in track_features: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        STAB_LOG_ERROR("Standard exception in track_features: %s", e.what());
        return false;
    } catch (...) {
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
        STAB_LOG_ERROR("OpenCV exception in estimate_transform: %s", e.what());
        return cv::Mat::eye(2, 3, CV_64F);
    } catch (const std::exception& e) {
        STAB_LOG_ERROR("Standard exception in estimate_transform: %s", e.what());
        return cv::Mat::eye(2, 3, CV_64F);
    } catch (...) {
        STAB_LOG_ERROR("Unknown exception in estimate_transform");
        return cv::Mat::eye(2, 3, CV_64F);
    }
}

cv::Mat StabilizerCore::smooth_transforms() {
    if (params_.use_high_pass_filter) {
        return smooth_high_pass_filter(transforms_, params_.high_pass_attenuation);
    }
    
    if (params_.use_directional_smoothing) {
        cv::Vec2d direction = cv::Vec2d(1.0, 0.3);
        return smooth_directional(transforms_, direction);
    }
    
    return smooth_transforms_optimized();
}

cv::Mat StabilizerCore::smooth_transforms_optimized() {
    if (transforms_.empty()) {
        return cv::Mat::eye(2, 3, CV_64F);
    }

    const size_t size = transforms_.size();
    cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
    
    // Use platform-specific optimization for transform averaging
    #ifndef BUILD_STANDALONE
    if (PlatformOptimization::is_arm64()) {
        // Use NEON-optimized transform averaging
        PlatformOptimization::NEON::TransformMatrix sum_matrix = PlatformOptimization::NEON::TransformMatrix::zero();
        
        for (const auto& t : transforms_) {
            const double* t_ptr = t.ptr<double>(0);
            PlatformOptimization::NEON::TransformMatrix t_matrix(
                t_ptr[0], t_ptr[1], t_ptr[2],
                t_ptr[3], t_ptr[4], t_ptr[5]
            );
            sum_matrix = PlatformOptimization::NEON::add(sum_matrix, t_matrix);
        }
        
        // Apply multiplication instead of division
        const float inv_size = 1.0f / static_cast<float>(size);
        PlatformOptimization::NEON::TransformMatrix avg_matrix = PlatformOptimization::NEON::mul_scalar(sum_matrix, inv_size);
        
        // Copy result back to OpenCV matrix
        double* ptr = smoothed.ptr<double>(0);
        ptr[0] = avg_matrix.row0.data[0]; ptr[1] = avg_matrix.row0.data[1]; ptr[2] = avg_matrix.row0.data[2];
        ptr[3] = avg_matrix.row1.data[0]; ptr[4] = avg_matrix.row1.data[1]; ptr[5] = avg_matrix.row1.data[2];
        
        return smoothed;
    }
    #endif
    
    // Fallback to original optimized implementation
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

void StabilizerCore::filter_transforms(std::vector<cv::Mat>& transforms) {
    // Remove outliers using simple threshold-based filtering
    if (transforms.size() < 3) return;
    
    double sum_x = 0.0, sum_y = 0.0;
    for (const auto& t : transforms) {
        const double* ptr = t.ptr<double>(0);
        sum_x += ptr[0] + ptr[3];
        sum_y += ptr[1] + ptr[4];
    }
    
    double mean_x = sum_x / (transforms.size() * 2);
    double mean_y = sum_y / (transforms.size() * 2);
    
    const double threshold = 2.0; // Standard deviations
    std::vector<cv::Mat> filtered;
    filtered.reserve(transforms.size());
    
    for (const auto& t : transforms) {
        const double* ptr = t.ptr<double>(0);
        double dx = std::abs(ptr[0] + ptr[3] - mean_x);
        double dy = std::abs(ptr[1] + ptr[4] - mean_y);
        
        if (dx < threshold && dy < threshold) {
            filtered.push_back(t);
        }
    }
    
    transforms = std::move(filtered);
}

bool StabilizerCore::should_refresh_features(float success_rate, int frames_since_refresh) {
    // Use lookup table for threshold comparisons to avoid branching
    static const float refresh_thresholds[] = {0.3f, 0.5f, 0.7f};
    static const int refresh_intervals[] = {10, 30, 50};

    if (frames_since_refresh >= refresh_intervals[1]) {
        return success_rate < refresh_thresholds[1];
    }

    return false;
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
        STAB_LOG_ERROR("OpenCV exception in apply_transform: %s", e.what());
        return frame.clone();
    } catch (const std::exception& e) {
        STAB_LOG_ERROR("Standard exception in apply_transform: %s", e.what());
        return frame.clone();
    } catch (...) {
        STAB_LOG_ERROR("Unknown exception in apply_transform");
        return frame.clone();
    }
}

cv::Mat StabilizerCore::apply_edge_handling(const cv::Mat& frame, EdgeMode mode) {
    try {
        switch (mode) {
            case EdgeMode::Padding:
                // Padding mode: Return frame as-is with black borders
                return frame;

            case EdgeMode::Crop: {
                // Crop mode: Remove black borders from edges
                // Convert to grayscale for better border detection
                cv::Mat gray;
                if (frame.channels() == 4) {
                    cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
                } else if (frame.channels() == 3) {
                    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                } else {
                    gray = frame;
                }

                // Threshold to identify black areas
                cv::Mat thresholded;
                cv::threshold(gray, thresholded, 10, 255, cv::THRESH_BINARY);

                // Find bounding box of non-black content
                cv::Mat col_sum, row_sum;
                cv::reduce(thresholded, col_sum, 0, cv::REDUCE_SUM, CV_32S);
                cv::reduce(thresholded, row_sum, 1, cv::REDUCE_SUM, CV_32S);

                int left = 0, right = thresholded.cols - 1;
                int top = 0, bottom = thresholded.rows - 1;

                // Find left edge
                while (left < right && col_sum.at<int>(0, left) == 0) left++;
                // Find right edge
                while (right > left && col_sum.at<int>(0, right) == 0) right--;
                // Find top edge
                while (top < bottom && row_sum.at<int>(top, 0) == 0) top++;
                // Find bottom edge
                while (bottom > top && row_sum.at<int>(bottom, 0) == 0) bottom--;

                // Ensure crop region is valid
                if (left >= right || top >= bottom) {
                    return frame; // No valid crop region, return original
                }

                // Crop the frame
                cv::Rect crop_rect(left, top, right - left, bottom - top);
                if (crop_rect.x >= 0 && crop_rect.y >= 0 &&
                    crop_rect.x + crop_rect.width <= frame.cols &&
                    crop_rect.y + crop_rect.height <= frame.rows) {
                    return frame(crop_rect).clone();
                }
                return frame;
            }

            case EdgeMode::Scale: {
                // Scale mode: Scale frame to fill original dimensions
                // Convert to grayscale for border detection
                cv::Mat gray;
                if (frame.channels() == 4) {
                    cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
                } else if (frame.channels() == 3) {
                    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
                } else {
                    gray = frame;
                }

                // Threshold to identify black areas
                cv::Mat thresholded;
                cv::threshold(gray, thresholded, 10, 255, cv::THRESH_BINARY);

                // Find bounding box of non-black content
                cv::Mat col_sum, row_sum;
                cv::reduce(thresholded, col_sum, 0, cv::REDUCE_SUM, CV_32S);
                cv::reduce(thresholded, row_sum, 1, cv::REDUCE_SUM, CV_32S);

                int left = 0, right = thresholded.cols - 1;
                int top = 0, bottom = thresholded.rows - 1;

                // Find edges
                while (left < right && col_sum.at<int>(0, left) == 0) left++;
                while (right > left && col_sum.at<int>(0, right) == 0) right--;
                while (top < bottom && row_sum.at<int>(top, 0) == 0) top++;
                while (bottom > top && row_sum.at<int>(bottom, 0) == 0) bottom--;

                // Ensure crop region is valid
                if (left >= right || top >= bottom) {
                    return frame; // No valid crop region, return original
                }

                // Calculate scale factor to fill original frame
                int content_width = right - left;
                int content_height = bottom - top;
                double scale_x = static_cast<double>(frame.cols) / content_width;
                double scale_y = static_cast<double>(frame.rows) / content_height;
                double scale = std::min(scale_x, scale_y);

                // Scale the frame
                cv::Mat scaled;
                cv::resize(frame, scaled, cv::Size(), scale, scale, cv::INTER_LINEAR);

                // Center the scaled frame
                cv::Mat result(frame.size(), frame.type(), cv::Scalar(0, 0, 0, 255));
                int offset_x = (frame.cols - scaled.cols) / 2;
                int offset_y = (frame.rows - scaled.rows) / 2;
                cv::Rect roi(offset_x, offset_y, scaled.cols, scaled.rows);
                scaled.copyTo(result(roi));

                return result;
            }

            default:
                return frame;
        }
    } catch (const cv::Exception& e) {
        STAB_LOG_ERROR("OpenCV exception in apply_edge_handling: %s", e.what());
        return frame.clone();
    } catch (const std::exception& e) {
        STAB_LOG_ERROR("Standard exception in apply_edge_handling: %s", e.what());
        return frame.clone();
    } catch (...) {
        STAB_LOG_ERROR("Unknown exception in apply_edge_handling");
        return frame.clone();
    }
}

void StabilizerCore::update_parameters(const StabilizerCore::StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    params_ = params;
}

void StabilizerCore::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    first_frame_ = true;
    prev_gray_ = cv::Mat(height_, width_, CV_8UC1);
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
    metrics_ = {};
    consecutive_tracking_failures_ = 0;
    frames_since_last_refresh_ = 0;
}

void StabilizerCore::clear_state() {
    reset();
}

StabilizerCore::PerformanceMetrics StabilizerCore::get_performance_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

const std::deque<cv::Mat>& StabilizerCore::get_current_transforms() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return transforms_;
}

cv::Mat StabilizerCore::smooth_high_pass_filter(const std::deque<cv::Mat>& transforms, 
                                              double attenuation) {
    if (transforms.empty()) {
        return cv::Mat::eye(2, 3, CV_64F);
    }
    
    cv::Mat smoothed = smooth_transforms_optimized();
    cv::Mat high_freq = transforms.back() - smoothed;
    
    cv::Mat result = smoothed + high_freq * attenuation;
    return result;
}

cv::Mat StabilizerCore::smooth_directional(const std::deque<cv::Mat>& transforms, 
                                       const cv::Vec2d& direction) {
    if (transforms.empty()) {
        return cv::Mat::eye(2, 3, CV_64F);
    }
    
    cv::Mat result = cv::Mat::zeros(2, 3, CV_64F);
    double* ptr = result.ptr<double>(0);
    
    for (const auto& t : transforms) {
        if (t.empty() || t.rows < 2 || t.cols < 3) {
            continue;
        }
        
        const double* t_ptr = t.ptr<double>(0);
        
        double parallel_mag = t_ptr[2] * direction[0] + t_ptr[5] * direction[1];
        double perp_mag = t_ptr[2] * direction[1] - t_ptr[5] * direction[0];
        
        ptr[0] += t_ptr[0] * 0.9;
        ptr[1] += t_ptr[1] * 0.9;
        ptr[2] += t_ptr[2] * 0.8 + parallel_mag * 0.1;
        ptr[3] += t_ptr[3] * 0.9;
        ptr[4] += t_ptr[4] * 0.9;
        ptr[5] += t_ptr[5] * 0.8 + perp_mag * 0.1;
    }
    
    double inv_count = 1.0 / static_cast<double>(transforms.size());
    for (int i = 0; i < 6; ++i) {
        ptr[i] *= inv_count;
    }
    
    return result;
}

bool StabilizerCore::is_ready() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return width_ > 0 && height_ > 0;
}

std::string StabilizerCore::get_last_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

StabilizerCore::StabilizerParams StabilizerCore::get_current_params() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return params_;
}

 bool StabilizerCore::validate_parameters(const StabilizerCore::StabilizerParams& params) {
    if (params.smoothing_radius < Smoothing::MIN_RADIUS || params.smoothing_radius > Smoothing::MAX_RADIUS) {
        return false;
    }
    if (params.max_correction < Correction::MIN_MAX || params.max_correction > Correction::MAX_MAX) {
        return false;
    }
    if (params.feature_count < Features::MIN_COUNT || params.feature_count > Features::MAX_COUNT) {
        return false;
    }
    if (params.quality_level < Quality::MIN_LEVEL || params.quality_level > Quality::MAX_LEVEL) {
        return false;
    }
    if (params.min_distance < Distance::MIN || params.min_distance > Distance::MAX) {
        return false;
    }
    if (params.block_size < Block::MIN_SIZE || params.block_size > Block::MAX_SIZE) {
        return false;
    }
    if (params.k < Harris::MIN_K || params.k > Harris::MAX_K) {
        return false;
    }

    if (params.optical_flow_pyramid_levels < OpticalFlow::MIN_PYRAMID_LEVELS || params.optical_flow_pyramid_levels > OpticalFlow::MAX_PYRAMID_LEVELS) {
        return false;
    }
    if (params.optical_flow_window_size < OpticalFlow::MIN_WINDOW_SIZE || params.optical_flow_window_size > OpticalFlow::MAX_WINDOW_SIZE ||
        params.optical_flow_window_size % 2 == 0) {
        return false;
    }
    if (params.feature_refresh_threshold < 0.0f || params.feature_refresh_threshold > 1.0f) {
        return false;
    }
    if (params.adaptive_feature_min < Features::MIN_COUNT || params.adaptive_feature_min > params.adaptive_feature_max) {
        return false;
    }
    if (params.adaptive_feature_max < params.adaptive_feature_min || params.adaptive_feature_max > Features::MAX_COUNT) {
        return false;
    }

    return true;
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
    params.optical_flow_pyramid_levels = OpticalFlow::DEFAULT_PYRAMID_LEVELS;
    params.optical_flow_window_size = OpticalFlow::DEFAULT_WINDOW_SIZE;
    params.feature_refresh_threshold = AdaptiveFeatures::GAMING_REFRESH;
    params.adaptive_feature_min = AdaptiveFeatures::GAMING_MIN;
    params.adaptive_feature_max = AdaptiveFeatures::GAMING_MAX;
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
    params.optical_flow_pyramid_levels = OpticalFlow::DEFAULT_PYRAMID_LEVELS;
    params.optical_flow_window_size = OpticalFlow::DEFAULT_WINDOW_SIZE;
    params.feature_refresh_threshold = AdaptiveFeatures::STREAMING_REFRESH;
    params.adaptive_feature_min = AdaptiveFeatures::STREAMING_MIN;
    params.adaptive_feature_max = AdaptiveFeatures::STREAMING_MAX;
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
    params.optical_flow_pyramid_levels = OpticalFlow::RECORDING_PYRAMID_LEVELS;
    params.optical_flow_window_size = OpticalFlow::RECORDING_WINDOW_SIZE;
    params.feature_refresh_threshold = AdaptiveFeatures::RECORDING_REFRESH;
    params.adaptive_feature_min = AdaptiveFeatures::RECORDING_MIN;
    params.adaptive_feature_max = AdaptiveFeatures::RECORDING_MAX;
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
    return true;
}

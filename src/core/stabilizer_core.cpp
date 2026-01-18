/*
 * OBS Stabilizer Core Implementation
 * Core stabilization algorithms using OpenCV
 */

#include "stabilizer_core.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

// Missing log level definitions
#ifndef LOG_INFO
#define LOG_INFO 200
#endif
#ifndef LOG_ERROR 
#define LOG_ERROR 400
#endif

// Missing obs_log function for core module
#ifndef obs_log
extern "C" void obs_log(int log_level, const char *format, ...);
#endif

// ============================================================================
// StabilizerCore Implementation
// ============================================================================

bool StabilizerCore::initialize(uint32_t width, uint32_t height, const StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!validate_parameters(params)) {
        last_error_ = "Invalid parameters provided";
        return false;
    }
    
    if (width == 0 || height == 0) {
        last_error_ = "Invalid frame dimensions";
        return false;
    }
    
    width_ = width;
    height_ = height;
    params_ = params;
    first_frame_ = true;
    
    // Initialize OpenCV data structures
    prev_gray_.create(height, width, CV_8UC1);
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
    
    // Reset performance metrics
    metrics_ = PerformanceMetrics{};
    metrics_.last_frame_time = std::chrono::high_resolution_clock::now();
    
    last_error_.clear();
    return true;
}

cv::Mat StabilizerCore::process_frame(const cv::Mat& frame) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        if (!validate_frame(frame)) {
            last_error_ = "Invalid input frame";
            return frame;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check dimensions
        if (frame.cols != static_cast<int>(width_) || frame.rows != static_cast<int>(height_)) {
            // Reinitialize with new dimensions
            width_ = frame.cols;
            height_ = frame.rows;
            first_frame_ = true;
            prev_gray_.release();
            prev_pts_.clear();
            transforms_.clear();
            cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
        }
        
        cv::Mat gray;
        if (frame.channels() == 4) {
            cv::cvtColor(frame, gray, cv::COLOR_BGRA2GRAY);
        } else if (frame.channels() == 3) {
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = frame.clone();
        }
        
        if (first_frame_) {
            prev_gray_ = gray.clone();
            
            // Detect initial feature points
            if (!detect_features(gray, prev_pts_)) {
                last_error_ = "Failed to detect initial features";
                return frame;
            }
            
            first_frame_ = false;
            return frame;
        }
        
        // Track features from previous frame to current
        std::vector<cv::Point2f> curr_pts;
        if (!track_features(prev_gray_, gray, prev_pts_, curr_pts)) {
            // Tracking failed, refresh features
            prev_gray_ = gray.clone();
            if (!detect_features(gray, prev_pts_)) {
                last_error_ = "Feature detection failed";
                return frame;
            }
            return frame;
        }
        
        // Estimate transformation between frames
        cv::Mat transform = estimate_transform(prev_pts_, curr_pts);
        if (transform.empty()) {
            // Transform estimation failed, refresh features
            prev_gray_ = gray.clone();
            prev_pts_ = curr_pts;
            return frame;
        }
        
        // Store transformation
        transforms_.push_back(transform);
        if (static_cast<int>(transforms_.size()) > params_.smoothing_radius) {
            transforms_.pop_front();
        }
        
        // Smooth transformations
        cv::Mat smoothed_transform = smooth_transforms();
        
        // Apply stabilized transformation
        cv::Mat stabilized_frame = apply_transform(frame, smoothed_transform);
        
        // Update tracking data
        prev_gray_ = gray.clone();
        prev_pts_ = curr_pts;
        
        // Update performance metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time = duration.count() / 1000.0;
        log_performance(processing_time);
        
        last_error_.clear();
        return stabilized_frame;
        
    } catch (const cv::Exception& e) {
        last_error_ = "OpenCV error: " + std::string(e.what());
        return frame;
    } catch (const std::exception& e) {
        last_error_ = "Error: " + std::string(e.what());
        return frame;
    }
}

void StabilizerCore::update_parameters(const StabilizerParams& params) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (validate_parameters(params)) {
        params_ = params;
        
        // Adjust transforms queue if smoothing radius changed
        while (static_cast<int>(transforms_.size()) > params_.smoothing_radius) {
            transforms_.pop_front();
        }
        
        last_error_.clear();
    } else {
        last_error_ = "Invalid parameters provided";
    }
}

void StabilizerCore::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    clear_state();
}

StabilizerCore::PerformanceMetrics StabilizerCore::get_performance_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

bool StabilizerCore::is_ready() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return width_ > 0 && height_ > 0 && !first_frame_;
}

std::string StabilizerCore::get_last_error() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

// Private methods

bool StabilizerCore::detect_features(const cv::Mat& gray, std::vector<cv::Point2f>& points) {
    try {
        std::vector<cv::Point2f> detected_points;
        
        if (params_.use_harris) {
            cv::goodFeaturesToTrack(gray, detected_points, params_.feature_count,
                                   params_.quality_level, params_.min_distance,
                                   cv::noArray(), params_.block_size, true, params_.k);
        } else {
            cv::goodFeaturesToTrack(gray, detected_points, params_.feature_count,
                                   params_.quality_level, params_.min_distance,
                                   cv::noArray(), params_.block_size, false, 0.0);
        }
        
        if (detected_points.empty()) {
            return false;
        }
        
        // Filter points to be away from image borders
        const int border_margin = 20;
        points.clear();
        for (const auto& pt : detected_points) {
            if (pt.x > border_margin && pt.x < gray.cols - border_margin &&
                pt.y > border_margin && pt.y < gray.rows - border_margin) {
                points.push_back(pt);
            }
        }
        
        return !points.empty();
        
    } catch (const cv::Exception& e) {
        last_error_ = "Feature detection error: " + std::string(e.what());
        return false;
    }
}

bool StabilizerCore::track_features(const cv::Mat& prev_gray, const cv::Mat& curr_gray,
                                   std::vector<cv::Point2f>& prev_pts, std::vector<cv::Point2f>& curr_pts) {
    try {
        if (prev_pts.empty()) {
            return false;
        }
        
        std::vector<uchar> status;
        std::vector<float> err;
        
        cv::calcOpticalFlowPyrLK(prev_gray, curr_gray, prev_pts, curr_pts,
                                 status, err, cv::Size(21, 21), 3,
                                 cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 30, 0.01),
                                 0, 0.001);
        
        // Filter out points that failed to track
        std::vector<cv::Point2f> good_prev_pts, good_curr_pts;
        for (size_t i = 0; i < status.size(); ++i) {
            if (status[i] && err[i] < 50.0) {
                good_prev_pts.push_back(prev_pts[i]);
                good_curr_pts.push_back(curr_pts[i]);
            }
        }
        
        if (good_prev_pts.size() < static_cast<size_t>(params_.feature_count * 0.3)) {
            // Too few points tracked successfully
            return false;
        }
        
        prev_pts = good_prev_pts;
        curr_pts = good_curr_pts;
        
        return true;
        
    } catch (const cv::Exception& e) {
        last_error_ = "Feature tracking error: " + std::string(e.what());
        return false;
    }
}

cv::Mat StabilizerCore::estimate_transform(const std::vector<cv::Point2f>& prev_pts,
                                          const std::vector<cv::Point2f>& curr_pts) {
    try {
        if (prev_pts.size() < 4 || curr_pts.size() < 4) {
            return cv::Mat();
        }
        
        cv::Mat inliers;
        cv::Mat transform = cv::estimateAffinePartial2D(prev_pts, curr_pts, inliers,
                                                       cv::RANSAC, 3.0);
        
        if (transform.empty()) {
            return cv::Mat();
        }
        
        // Convert to 3x3 homogeneous matrix
        cv::Mat homogeneous_transform = cv::Mat::eye(3, 3, CV_64F);
        transform.copyTo(homogeneous_transform(cv::Rect(0, 0, 3, 2)));
        
        return homogeneous_transform;
        
    } catch (const cv::Exception& e) {
        last_error_ = "Transform estimation error: " + std::string(e.what());
        return cv::Mat();
    }
}

cv::Mat StabilizerCore::smooth_transforms() {
    try {
        if (transforms_.empty()) {
            return cv::Mat::eye(3, 3, CV_64F);
        }
        
        // Calculate moving average of transformations
        cv::Mat avg_transform = cv::Mat::zeros(3, 3, CV_64F);
        
        for (const auto& transform : transforms_) {
            avg_transform += transform;
        }
        
        avg_transform /= static_cast<double>(transforms_.size());
        
        // Apply correction limits
        double translation_x = avg_transform.at<double>(0, 2);
        double translation_y = avg_transform.at<double>(1, 2);
        
        double max_translation = params_.max_correction;
        double distance = std::sqrt(translation_x * translation_x + translation_y * translation_y);
        
        if (distance > max_translation) {
            double scale = max_translation / distance;
            avg_transform.at<double>(0, 2) *= scale;
            avg_transform.at<double>(1, 2) *= scale;
        }
        
        return avg_transform;
        
    } catch (const cv::Exception& e) {
        last_error_ = "Transform smoothing error: " + std::string(e.what());
        return cv::Mat::eye(3, 3, CV_64F);
    }
}

cv::Mat StabilizerCore::apply_transform(const cv::Mat& frame, const cv::Mat& transform) {
    try {
        if (transform.empty()) {
            return frame.clone();
        }
        
        cv::Mat stabilized;
        cv::warpAffine(frame, stabilized, transform(cv::Rect(0, 0, 3, 2)),
                      frame.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT);
        
        return stabilized;
        
    } catch (const cv::Exception& e) {
        last_error_ = "Transform application error: " + std::string(e.what());
        return frame.clone();
    }
}

void StabilizerCore::log_performance(double processing_time) {
    metrics_.frame_count++;
    metrics_.avg_processing_time = (metrics_.avg_processing_time * (metrics_.frame_count - 1) + processing_time) / metrics_.frame_count;
    
    if (params_.debug_mode && metrics_.frame_count % 60 == 0) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Stabilizer Performance: " << metrics_.avg_processing_time << "ms avg, "
            << "Frame " << metrics_.frame_count;
        obs_log(LOG_INFO, "%s", oss.str().c_str());
    }
}

bool StabilizerCore::validate_frame(const cv::Mat& frame) {
    return !frame.empty() && frame.channels() >= 1 && frame.channels() <= 4;
}

void StabilizerCore::clear_state() {
    first_frame_ = true;
    prev_gray_.release();
    prev_pts_.clear();
    transforms_.clear();
    cumulative_transform_ = cv::Mat::eye(3, 3, CV_64F);
    metrics_ = PerformanceMetrics{};
    last_error_.clear();
}

// Static methods

bool StabilizerCore::validate_parameters(const StabilizerParams& params) {
    return ParameterValidator::validate_all(params);
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_gaming() {
    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = PRESETS::GAMING::SMOOTHING_RADIUS;
    params.max_correction = PRESETS::GAMING::MAX_CORRECTION;
    params.feature_count = PRESETS::GAMING::FEATURE_COUNT;
    params.quality_level = PRESETS::GAMING::QUALITY_LEVEL;
    params.min_distance = PRESETS::GAMING::MIN_DISTANCE;
    params.block_size = OPENCV_PARAMS::BLOCK_SIZE_DEFAULT;
    params.use_harris = OPENCV_PARAMS::USE_HARRIS_DEFAULT;
    params.k = OPENCV_PARAMS::HARRIS_K_DEFAULT;
    return params;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_streaming() {
    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = PRESETS::STREAMING::SMOOTHING_RADIUS;
    params.max_correction = PRESETS::STREAMING::MAX_CORRECTION;
    params.feature_count = PRESETS::STREAMING::FEATURE_COUNT;
    params.quality_level = PRESETS::STREAMING::QUALITY_LEVEL;
    params.min_distance = PRESETS::STREAMING::MIN_DISTANCE;
    params.block_size = OPENCV_PARAMS::BLOCK_SIZE_DEFAULT;
    params.use_harris = OPENCV_PARAMS::USE_HARRIS_DEFAULT;
    params.k = OPENCV_PARAMS::HARRIS_K_DEFAULT;
    return params;
}

StabilizerCore::StabilizerParams StabilizerCore::get_preset_recording() {
    StabilizerCore::StabilizerParams params;
    params.smoothing_radius = PRESETS::RECORDING::SMOOTHING_RADIUS;
    params.max_correction = PRESETS::RECORDING::MAX_CORRECTION;
    params.feature_count = PRESETS::RECORDING::FEATURE_COUNT;
    params.quality_level = PRESETS::RECORDING::QUALITY_LEVEL;
    params.min_distance = PRESETS::RECORDING::MIN_DISTANCE;
    params.block_size = OPENCV_PARAMS::BLOCK_SIZE_DEFAULT;
    params.use_harris = OPENCV_PARAMS::USE_HARRIS_DEFAULT;
    params.k = OPENCV_PARAMS::HARRIS_K_DEFAULT;
    return params;
}

// ============================================================================
// TransformMatrix Implementation
// ============================================================================

TransformMatrix::TransformMatrix() : matrix_(cv::Mat::eye(3, 3, CV_64F)) {}

TransformMatrix::TransformMatrix(const cv::Mat& matrix) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (matrix.rows == 3 && matrix.cols == 3) {
        matrix_ = matrix.clone();
    } else {
        matrix_ = cv::Mat::eye(3, 3, CV_64F);
    }
}

TransformMatrix TransformMatrix::operator*(const TransformMatrix& other) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> other_lock(other.mutex_);
    
    cv::Mat result = matrix_ * other.matrix_;
    return TransformMatrix(result);
}

TransformMatrix TransformMatrix::inverse() const {
    std::lock_guard<std::mutex> lock(mutex_);
    cv::Mat inv_matrix;
    cv::invert(matrix_, inv_matrix);
    return TransformMatrix(inv_matrix);
}

bool TransformMatrix::is_valid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !matrix_.empty() && matrix_.rows == 3 && matrix_.cols == 3;
}

TransformMatrix TransformMatrix::identity() {
    return TransformMatrix();
}

TransformMatrix TransformMatrix::from_affine(double tx, double ty, double angle, double scale) {
    cv::Mat transform = cv::Mat::eye(3, 3, CV_64F);
    
    // Apply rotation and scale
    double cos_a = std::cos(angle) * scale;
    double sin_a = std::sin(angle) * scale;
    
    transform.at<double>(0, 0) = cos_a;
    transform.at<double>(0, 1) = -sin_a;
    transform.at<double>(1, 0) = sin_a;
    transform.at<double>(1, 1) = cos_a;
    
    // Apply translation
    transform.at<double>(0, 2) = tx;
    transform.at<double>(1, 2) = ty;
    
    return TransformMatrix(transform);
}

// ============================================================================
// ParameterValidator Implementation
// ============================================================================

bool ParameterValidator::validate_smoothing_radius(int value) {
    return value >= 5 && value <= 200;
}

bool ParameterValidator::validate_max_correction(float value) {
    return value >= 1.0f && value <= 100.0f;
}

bool ParameterValidator::validate_feature_count(int value) {
    return value >= 50 && value <= 2000;
}

bool ParameterValidator::validate_quality_level(float value) {
    return value >= 0.001f && value <= 0.1f;
}

bool ParameterValidator::validate_min_distance(float value) {
    return value >= 1.0f && value <= 200.0f;
}

bool ParameterValidator::validate_block_size(int value) {
    return value >= 3 && value <= 31 && (value % 2) == 1;
}

bool ParameterValidator::validate_harris_k(float value) {
    return value >= 0.01f && value <= 0.1f;
}

bool ParameterValidator::validate_all(const StabilizerCore::StabilizerParams& params) {
    return validate_smoothing_radius(params.smoothing_radius) &&
           validate_max_correction(params.max_correction) &&
           validate_feature_count(params.feature_count) &&
           validate_quality_level(params.quality_level) &&
           validate_min_distance(params.min_distance) &&
           validate_block_size(params.block_size) &&
           validate_harris_k(params.k);
}

std::vector<std::string> ParameterValidator::get_validation_errors(const StabilizerCore::StabilizerParams& params) {
    std::vector<std::string> errors;
    
    if (!validate_smoothing_radius(params.smoothing_radius)) {
        errors.push_back("Smoothing radius must be between 5 and 200");
    }
    if (!validate_max_correction(params.max_correction)) {
        errors.push_back("Max correction must be between 1.0 and 100.0");
    }
    if (!validate_feature_count(params.feature_count)) {
        errors.push_back("Feature count must be between 50 and 2000");
    }
    if (!validate_quality_level(params.quality_level)) {
        errors.push_back("Quality level must be between 0.001 and 0.1");
    }
    if (!validate_min_distance(params.min_distance)) {
        errors.push_back("Min distance must be between 1.0 and 200.0");
    }
    if (!validate_block_size(params.block_size)) {
        errors.push_back("Block size must be between 3 and 31 (odd numbers only)");
    }
    if (!validate_harris_k(params.k)) {
        errors.push_back("Harris K parameter must be between 0.01 and 0.1");
    }
    
    return errors;
}

// ============================================================================
// ErrorHandler Implementation
// ============================================================================

thread_local std::string ErrorHandler::last_error_;

std::string ErrorHandler::format_error(ErrorType type, const std::string& message) {
    std::string prefix;
    switch (type) {
        case ErrorType::Initialization: prefix = "Initialization Error"; break;
        case ErrorType::Memory: prefix = "Memory Error"; break;
        case ErrorType::OpenCV: prefix = "OpenCV Error"; break;
        case ErrorType::Parameter: prefix = "Parameter Error"; break;
        case ErrorType::Processing: prefix = "Processing Error"; break;
        case ErrorType::Thread: prefix = "Thread Error"; break;
    }
    
    return prefix + ": " + message;
}

void ErrorHandler::log_error(ErrorType type, const std::string& message) {
    last_error_ = format_error(type, message);
    obs_log(LOG_ERROR, "%s", last_error_.c_str());
}

std::string ErrorHandler::get_last_error() {
    return last_error_;
}
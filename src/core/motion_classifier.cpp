#include "core/motion_classifier.hpp"
#include "logging.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <limits>

// Logging macros
#ifndef STAB_LOG_ERROR
#define STAB_LOG_ERROR(...) CORE_LOG_ERROR(__VA_ARGS__)
#endif
#ifndef STAB_LOG_WARNING
#define STAB_LOG_WARNING(...) CORE_LOG_WARNING(__VA_ARGS__)
#endif
#ifndef STAB_LOG_INFO
#define STAB_LOG_INFO(...) CORE_LOG_INFO(__VA_ARGS__)
#endif

using namespace AdaptiveStabilization;

MotionClassifier::MotionClassifier(size_t window_size, double sensitivity)
    : window_size_(window_size)
    , sensitivity_(sensitivity)
    , current_type_(MotionType::Static)
    , current_metrics_()
{
}

std::string MotionClassifier::motion_type_to_string(MotionType type) {
    switch (type) {
        case MotionType::Static: return "Static";
        case MotionType::SlowMotion: return "Slow Motion";
        case MotionType::FastMotion: return "Fast Motion";
        case MotionType::CameraShake: return "Camera Shake";
        case MotionType::PanZoom: return "Pan/Zoom";
        default: return "Unknown";
    }
}

double MotionClassifier::calculate_magnitude(const cv::Mat& transform) const {
    if (transform.empty() || transform.rows < 2 || transform.cols < 3) {
        return 0.0;
    }
    
    const double* ptr = transform.ptr<double>(0);
    
    double translation_x = ptr[2];
    double translation_y = ptr[5];
    
    double scale_x = ptr[0];
    double scale_y = ptr[4];
    
    double rotation = std::atan2(ptr[1], ptr[0]);
    
    double translation_magnitude = std::sqrt(translation_x * translation_x + 
                                         translation_y * translation_y);
    
    double scale_deviation = std::abs(scale_x - 1.0) + std::abs(scale_y - 1.0);
    double rotation_deviation = std::abs(rotation);
    
    return translation_magnitude + scale_deviation * 100.0 + rotation_deviation * 200.0;
}

double MotionClassifier::calculate_mean_magnitude(const std::deque<cv::Mat>& transforms) const {
    if (transforms.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (const auto& t : transforms) {
        sum += calculate_magnitude(t);
    }
    
    return sum / static_cast<double>(transforms.size());
}

double MotionClassifier::calculate_variance_magnitude(const std::deque<cv::Mat>& transforms, 
                                                     double mean) const {
    if (transforms.empty() || transforms.size() < 2) {
        return 0.0;
    }
    
    double sum_sq_diff = 0.0;
    for (const auto& t : transforms) {
        double mag = calculate_magnitude(t);
        double diff = mag - mean;
        sum_sq_diff += diff * diff;
    }
    
    return sum_sq_diff / static_cast<double>(transforms.size());
}

double MotionClassifier::calculate_directional_variance(const std::deque<cv::Mat>& transforms) const {
    if (transforms.empty()) {
        return 0.0;
    }
    
    double sum_dx = 0.0, sum_dy = 0.0;
    for (const auto& t : transforms) {
        if (!t.empty() && t.rows >= 2 && t.cols >= 3) {
            const double* ptr = t.ptr<double>(0);
            sum_dx += ptr[2];
            sum_dy += ptr[5];
        }
    }
    
    double mean_dx = sum_dx / static_cast<double>(transforms.size());
    double mean_dy = sum_dy / static_cast<double>(transforms.size());
    
    double var_dx = 0.0, var_dy = 0.0;
    for (const auto& t : transforms) {
        if (!t.empty() && t.rows >= 2 && t.cols >= 3) {
            const double* ptr = t.ptr<double>(0);
            double diff_dx = ptr[2] - mean_dx;
            double diff_dy = ptr[5] - mean_dy;
            var_dx += diff_dx * diff_dx;
            var_dy += diff_dy * diff_dy;
        }
    }
    
    var_dx /= static_cast<double>(transforms.size());
    var_dy /= static_cast<double>(transforms.size());
    
    return std::sqrt(var_dx + var_dy);
}

double MotionClassifier::calculate_consistency_score(const std::deque<cv::Mat>& transforms) const {
    if (transforms.empty()) {
        return 0.0;
    }
    
    if (transforms.size() < 2) {
        return 1.0;
    }
    
    double dot_sum = 0.0;
    double mag_sum = 0.0;
    
    for (size_t i = 1; i < transforms.size(); ++i) {
        const auto& t_prev = transforms[i - 1];
        const auto& t_curr = transforms[i];
        
        if (t_prev.empty() || t_curr.empty() || 
            t_prev.rows < 2 || t_prev.cols < 3 ||
            t_curr.rows < 2 || t_curr.cols < 3) {
            continue;
        }
        
        const double* ptr_prev = t_prev.ptr<double>(0);
        const double* ptr_curr = t_curr.ptr<double>(0);
        
        double dx_prev = ptr_prev[2];
        double dy_prev = ptr_prev[5];
        double dx_curr = ptr_curr[2];
        double dy_curr = ptr_curr[5];
        
        double dot = dx_prev * dx_curr + dy_prev * dy_curr;
        double mag_prev = std::sqrt(dx_prev * dx_prev + dy_prev * dy_prev);
        double mag_curr = std::sqrt(dx_curr * dx_curr + dy_curr * dy_curr);
        
        if (mag_prev > 0.001 && mag_curr > 0.001) {
            dot_sum += dot / (mag_prev * mag_curr);
            mag_sum += 1.0;
        }
    }
    
    return mag_sum > 0.0 ? dot_sum / mag_sum : 0.0;
}

double MotionClassifier::calculate_frequency_metrics(const std::deque<cv::Mat>& transforms) const {
    if (transforms.size() < 6) {
        return 0.0;
    }
    
    double low_freq_energy = 0.0;
    double high_freq_energy = 0.0;
    
    for (size_t i = 2; i < transforms.size(); ++i) {
        if (i >= 2) {
            double mag = calculate_magnitude(transforms[i]);
            double mag_prev_1 = calculate_magnitude(transforms[i - 1]);
            double mag_prev_2 = calculate_magnitude(transforms[i - 2]);
            
            double diff_1 = mag - mag_prev_1;
            double diff_2 = mag_prev_1 - mag_prev_2;
            
            double high_freq = std::abs(diff_1 - diff_2);
            double low_freq = std::abs(mag - mag_prev_2) * 0.5;
            
            high_freq_energy += high_freq;
            low_freq_energy += low_freq;
        }
    }
    
    double total_energy = high_freq_energy + low_freq_energy;
    return total_energy > 0.001 ? high_freq_energy / total_energy : 0.0;
}

MotionMetrics MotionClassifier::calculate_metrics(const std::deque<cv::Mat>& transforms) {
    MotionMetrics metrics;
    metrics.transform_count = transforms.size();
    
    if (transforms.empty()) {
        return metrics;
    }
    
    metrics.mean_magnitude = calculate_mean_magnitude(transforms);
    metrics.variance_magnitude = calculate_variance_magnitude(transforms, metrics.mean_magnitude);
    metrics.directional_variance = calculate_directional_variance(transforms);
    metrics.consistency_score = calculate_consistency_score(transforms);
    metrics.high_frequency_ratio = calculate_frequency_metrics(transforms);
    
    return metrics;
}

MotionType MotionClassifier::classify_from_metrics(const MotionMetrics& metrics) const {
    double sensitivity_factor = sensitivity_;
    
    // Validate sensitivity factor input
    if (sensitivity_factor <= 0.0) {
        STAB_LOG_ERROR("Invalid sensitivity factor: %.6f in MotionClassifier::classify_from_metrics", sensitivity_factor);
        sensitivity_factor = 1.0;
    }
    if (sensitivity_factor > 100.0) {
        STAB_LOG_WARNING("Sensitivity factor too high: %.6f, clamping to 100.0 in MotionClassifier::classify_from_metrics", sensitivity_factor);
        sensitivity_factor = 100.0;
    }
    
    double static_threshold = 6.0 * sensitivity_factor;
    double slow_threshold = 15.0 * sensitivity_factor;
    double fast_threshold = 40.0 * sensitivity_factor;
    double variance_threshold = 3.0 * sensitivity_factor;
    double high_freq_threshold = 0.70 * sensitivity_factor;
    double consistency_threshold = 0.96 / sensitivity_factor;
    
    // Clamp thresholds to sensible limits
    static_threshold = std::clamp(static_threshold, 0.0, 100.0);
    slow_threshold = std::clamp(slow_threshold, 0.0, 100.0);
    fast_threshold = std::clamp(fast_threshold, 0.0, 100.0);
    variance_threshold = std::clamp(variance_threshold, 0.0, 100.0);
    high_freq_threshold = std::clamp(high_freq_threshold, 0.0, 1.0);
    consistency_threshold = std::clamp(consistency_threshold, 0.0, 1.0);
    
    // Log warning if thresholds were clamped
    double original_static_threshold = 6.0 * sensitivity_factor;
    double original_slow_threshold = 15.0 * sensitivity_factor;
    double original_fast_threshold = 40.0 * sensitivity_factor;
    double original_variance_threshold = 3.0 * sensitivity_factor;
    double original_high_freq_threshold = 0.70 * sensitivity_factor;
    double original_consistency_threshold = 0.96 / sensitivity_factor;
    
    if (static_threshold < original_static_threshold) {
        STAB_LOG_WARNING("Static threshold clamped from %.2f to %.2f in MotionClassifier::classify_from_metrics", 
                        original_static_threshold, static_threshold);
    }
    if (slow_threshold < original_slow_threshold) {
        STAB_LOG_WARNING("Slow threshold clamped from %.2f to %.2f in MotionClassifier::classify_from_metrics", 
                        original_slow_threshold, slow_threshold);
    }
    if (fast_threshold < original_fast_threshold) {
        STAB_LOG_WARNING("Fast threshold clamped from %.2f to %.2f in MotionClassifier::classify_from_metrics", 
                        original_fast_threshold, fast_threshold);
    }
    if (variance_threshold < original_variance_threshold) {
        STAB_LOG_WARNING("Variance threshold clamped from %.2f to %.2f in MotionClassifier::classify_from_metrics", 
                        original_variance_threshold, variance_threshold);
    }
    if (high_freq_threshold < original_high_freq_threshold) {
        STAB_LOG_WARNING("High frequency threshold clamped from %.2f to %.2f in MotionClassifier::classify_from_metrics", 
                        original_high_freq_threshold, high_freq_threshold);
    }
    if (consistency_threshold < original_consistency_threshold) {
        STAB_LOG_WARNING("Consistency threshold clamped from %.2f to %.2f in MotionClassifier::classify_from_metrics", 
                        original_consistency_threshold, consistency_threshold);
    }
    
    if (metrics.mean_magnitude < static_threshold &&
        metrics.variance_magnitude < variance_threshold) {
        return MotionType::Static;
    }
    
    if (metrics.high_frequency_ratio > high_freq_threshold) {
        return MotionType::CameraShake;
    }
    
    if (metrics.mean_magnitude >= slow_threshold &&
        metrics.mean_magnitude < fast_threshold) {
        return MotionType::FastMotion;
    }
    
    if (metrics.mean_magnitude >= static_threshold &&
        metrics.mean_magnitude < slow_threshold) {
        if (metrics.consistency_score > consistency_threshold &&
            metrics.directional_variance < 2.0) {
            return MotionType::PanZoom;
        }
        return MotionType::SlowMotion;
    }
    
    return MotionType::SlowMotion;
}

MotionType MotionClassifier::classify(const std::deque<cv::Mat>& transforms) {
    if (transforms.empty()) {
        return MotionType::Static;
    }
    
    size_t analysis_window = std::min(window_size_, transforms.size());
    std::deque<cv::Mat> window_transforms;
    
    auto start_it = transforms.end() - static_cast<ssize_t>(analysis_window);
    for (auto it = start_it; it != transforms.end(); ++it) {
        window_transforms.push_back(*it);
    }
    
    current_metrics_ = calculate_metrics(window_transforms);
    current_type_ = classify_from_metrics(current_metrics_);
    
    return current_type_;
}


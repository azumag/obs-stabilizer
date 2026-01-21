#pragma once

#include <opencv2/opencv.hpp>
#include <deque>
#include <string>

namespace AdaptiveStabilization {

enum class MotionType {
    Static,        // Minimal movement (< 1% max correction)
    SlowMotion,    // Gentle movement (1-5% max correction)
    FastMotion,    // Rapid movement (5-15% max correction)
    CameraShake,   // High-frequency jitter
    PanZoom        // Systematic directional motion
};

struct MotionMetrics {
    double mean_magnitude;           // Average movement per frame (%)
    double variance_magnitude;       // Variance of movement magnitudes (%)
    double directional_variance;      // Variance in movement direction
    double high_frequency_ratio;      // Ratio of high-frequency components
    double consistency_score;        // Directional consistency (0-1)
    size_t transform_count;          // Number of transforms analyzed
    
    MotionMetrics() 
        : mean_magnitude(0.0)
        , variance_magnitude(0.0)
        , directional_variance(0.0)
        , high_frequency_ratio(0.0)
        , consistency_score(0.0)
        , transform_count(0) {}
};

class MotionClassifier {
public:
    explicit MotionClassifier(size_t window_size = 30, double sensitivity = 1.0);
    ~MotionClassifier() = default;
    
    MotionType classify(const std::deque<cv::Mat>& transforms);
    MotionMetrics calculate_metrics(const std::deque<cv::Mat>& transforms);
    
    MotionType get_current_type() const { return current_type_; }
    const MotionMetrics& get_current_metrics() const { return current_metrics_; }
    
    void set_sensitivity(double sensitivity) { sensitivity_ = sensitivity; }
    double get_sensitivity() const { return sensitivity_; }
    
    static std::string motion_type_to_string(MotionType type);

private:
    double calculate_magnitude(const cv::Mat& transform) const;
    double calculate_mean_magnitude(const std::deque<cv::Mat>& transforms) const;
    double calculate_variance_magnitude(const std::deque<cv::Mat>& transforms, double mean) const;
    double calculate_directional_variance(const std::deque<cv::Mat>& transforms) const;
    double calculate_consistency_score(const std::deque<cv::Mat>& transforms) const;
    double calculate_frequency_metrics(const std::deque<cv::Mat>& transforms) const;
    
    MotionType classify_from_metrics(const MotionMetrics& metrics) const;
    
    size_t window_size_;
    double sensitivity_;
    MotionType current_type_;
    MotionMetrics current_metrics_;
};

} // namespace AdaptiveStabilization

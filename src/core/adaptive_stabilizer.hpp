#pragma once

#include "core/motion_classifier.hpp"
#include "core/stabilizer_core.hpp"
#include <map>

namespace AdaptiveStabilization {

struct AdaptiveConfig {
    // Static content parameters
    int static_smoothing = 8;
    double static_correction = 15.0;
    int static_features = 120;
    double static_quality = 0.015;
    
    // Slow motion parameters
    int slow_smoothing = 25;
    double slow_correction = 25.0;
    int slow_features = 175;
    double slow_quality = 0.010;
    
    // Fast motion parameters
    int fast_smoothing = 50;
    double fast_correction = 35.0;
    int fast_features = 250;
    double fast_quality = 0.010;
    
    // Camera shake parameters
    int shake_smoothing = 65;
    double shake_correction = 45.0;
    int shake_features = 350;
    double shake_quality = 0.005;
    
    // Pan/Zoom parameters
    int pan_smoothing = 15;
    double pan_correction = 20.0;
    int pan_features = 225;
    double pan_quality = 0.010;
    
    // Transition smoothing
    double transition_rate = 0.1; // 10% transition per frame (0.0-1.0)
};

class AdaptiveStabilizer {
public:
    explicit AdaptiveStabilizer(const AdaptiveConfig& config = AdaptiveConfig());
    ~AdaptiveStabilizer() = default;
    
    bool initialize(uint32_t width, uint32_t height, const StabilizerCore::StabilizerParams& params);
    cv::Mat process_frame(const cv::Mat& frame);
    
    void update_parameters(const StabilizerCore::StabilizerParams& params);
    void reset();
    
    bool is_ready() const;
    std::string get_last_error() const;
    StabilizerCore::PerformanceMetrics get_performance_metrics() const;
    
    void enable_adaptive(bool enable) { adaptive_enabled_ = enable; }
    bool is_adaptive_enabled() const { return adaptive_enabled_; }
    
    MotionType get_current_motion_type() const { return classifier_.get_current_type(); }
    const MotionMetrics& get_current_metrics() const { return classifier_.get_current_metrics(); }
    
    void set_config(const AdaptiveConfig& config) { config_ = config; }
    const AdaptiveConfig& get_config() const { return config_; }
    
    void set_motion_sensitivity(double sensitivity) { classifier_.set_sensitivity(sensitivity); }
    double get_motion_sensitivity() const { return classifier_.get_sensitivity(); }
    
private:
    void update_adaptive_parameters();
    void smooth_parameter_transition(StabilizerCore::StabilizerParams& target);
    StabilizerCore::StabilizerParams get_motion_params(MotionType type) const;
    
    StabilizerCore core_;
    MotionClassifier classifier_;
    AdaptiveConfig config_;
    
    bool adaptive_enabled_;
    bool initialized_;
    MotionType previous_motion_type_;
    StabilizerCore::StabilizerParams previous_params_;
    std::string last_error_;
};

} // namespace AdaptiveStabilization

#include "core/adaptive_stabilizer.hpp"
#include <algorithm>

using namespace AdaptiveStabilization;

AdaptiveStabilizer::AdaptiveStabilizer(const AdaptiveConfig& config)
    : core_()
    , classifier_(30, 1.0)
    , config_(config)
    , adaptive_enabled_(true)
    , initialized_(false)
    , previous_motion_type_(MotionType::Static)
    , previous_params_()
    , last_error_()
{
}

bool AdaptiveStabilizer::initialize(uint32_t width, uint32_t height, 
                                     const StabilizerCore::StabilizerParams& params) {
    if (!core_.initialize(width, height, params)) {
        last_error_ = core_.get_last_error();
        return false;
    }
    
    initialized_ = true;
    previous_params_ = params;
    return true;
}

cv::Mat AdaptiveStabilizer::process_frame(const cv::Mat& frame) {
    if (!initialized_ || frame.empty()) {
        return frame;
    }
    
    cv::Mat result = core_.process_frame(frame);
    
    if (adaptive_enabled_ && !result.empty()) {
        update_adaptive_parameters();
    }
    
    return result;
}

void AdaptiveStabilizer::update_parameters(const StabilizerCore::StabilizerParams& params) {
    if (initialized_) {
        core_.update_parameters(params);
        previous_params_ = params;
    }
}

void AdaptiveStabilizer::reset() {
    core_.reset();
    previous_motion_type_ = MotionType::Static;
    previous_params_ = StabilizerCore::StabilizerParams();
}

bool AdaptiveStabilizer::is_ready() const {
    return initialized_ && core_.is_ready();
}

std::string AdaptiveStabilizer::get_last_error() const {
    return last_error_.empty() ? core_.get_last_error() : last_error_;
}

StabilizerCore::PerformanceMetrics AdaptiveStabilizer::get_performance_metrics() const {
    return core_.get_performance_metrics();
}

void AdaptiveStabilizer::update_adaptive_parameters() {
    auto transforms = core_.get_current_transforms();
    
    if (transforms.size() < 5) {
        return;
    }
    
    MotionType current_type = classifier_.classify(transforms);
    
    if (current_type != previous_motion_type_) {
        StabilizerCore::StabilizerParams target_params = get_motion_params(current_type);
        
        if (previous_motion_type_ != MotionType::Static) {
            smooth_parameter_transition(target_params);
        } else {
            core_.update_parameters(target_params);
        }
        
        previous_motion_type_ = current_type;
        previous_params_ = target_params;
    }
}

void AdaptiveStabilizer::smooth_parameter_transition(StabilizerCore::StabilizerParams& target) {
    double rate = config_.transition_rate;
    
    StabilizerCore::StabilizerParams smoothed;
    smoothed.smoothing_radius = static_cast<int>(
        previous_params_.smoothing_radius + 
        (target.smoothing_radius - previous_params_.smoothing_radius) * rate);
    
    smoothed.max_correction = 
        previous_params_.max_correction + 
        (target.max_correction - previous_params_.max_correction) * rate;
    
    smoothed.feature_count = static_cast<int>(
        previous_params_.feature_count + 
        (target.feature_count - previous_params_.feature_count) * rate);
    
    smoothed.quality_level = 
        previous_params_.quality_level + 
        (target.quality_level - previous_params_.quality_level) * rate;
    
    smoothed.enabled = target.enabled;
    smoothed.use_harris = target.use_harris;
    smoothed.k = target.k;
    smoothed.min_distance = target.min_distance;
    smoothed.block_size = target.block_size;
    smoothed.optical_flow_window_size = target.optical_flow_window_size;
    smoothed.optical_flow_pyramid_levels = target.optical_flow_pyramid_levels;
    smoothed.feature_refresh_threshold = target.feature_refresh_threshold;
    
    core_.update_parameters(smoothed);
}

StabilizerCore::StabilizerParams AdaptiveStabilizer::get_motion_params(MotionType type) const {
    StabilizerCore::StabilizerParams params = core_.get_current_params();
    
    switch (type) {
        case MotionType::Static:
            params.smoothing_radius = config_.static_smoothing;
            params.max_correction = config_.static_correction;
            params.feature_count = config_.static_features;
            params.quality_level = config_.static_quality;
            params.feature_refresh_threshold = 0.9f;
            break;
            
        case MotionType::SlowMotion:
            params.smoothing_radius = config_.slow_smoothing;
            params.max_correction = config_.slow_correction;
            params.feature_count = config_.slow_features;
            params.quality_level = config_.slow_quality;
            params.feature_refresh_threshold = 0.7f;
            break;
            
        case MotionType::FastMotion:
            params.smoothing_radius = config_.fast_smoothing;
            params.max_correction = config_.fast_correction;
            params.feature_count = config_.fast_features;
            params.quality_level = config_.fast_quality;
            params.feature_refresh_threshold = 0.5f;
            break;
            
        case MotionType::CameraShake:
            params.smoothing_radius = config_.shake_smoothing;
            params.max_correction = config_.shake_correction;
            params.feature_count = config_.shake_features;
            params.quality_level = config_.shake_quality;
            params.feature_refresh_threshold = 0.4f;
            params.use_high_pass_filter = true;
            params.high_pass_attenuation = 0.3;
            break;
            
        case MotionType::PanZoom:
            params.smoothing_radius = config_.pan_smoothing;
            params.max_correction = config_.pan_correction;
            params.feature_count = config_.pan_features;
            params.quality_level = config_.pan_quality;
            params.feature_refresh_threshold = 0.6f;
            params.use_directional_smoothing = true;
            break;
    }
    
    return params;
}

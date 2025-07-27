/*
OBS Stabilizer Plugin - Debug and Diagnostic Features Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "stabilizer_core.hpp"
#include "logging_adapter.hpp"
#include <iomanip>
#include <sstream>
#include <mutex>
#include <atomic>

namespace obs_stabilizer {

#ifdef ENABLE_STABILIZATION

void StabilizerCore::update_detailed_metrics(const StabilizerMetrics& frame_metrics) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    // Thread-safe initialization of static variables
    static std::once_flag init_flag;
    static float avg_feature_detection = 0.0f;
    static float avg_optical_flow = 0.0f;
    static float avg_transform_calc = 0.0f;
    static float avg_smoothing = 0.0f;
    static std::atomic<int> frame_count{0};
    
    std::call_once(init_flag, []() {
        // Static variables are zero-initialized, no additional setup needed
    });
    
    frame_count++;
    
    // Exponential moving average with alpha = 0.1
    const float alpha = 0.1f;
    avg_feature_detection = (1.0f - alpha) * avg_feature_detection + alpha * frame_metrics.feature_detection_time_ms;
    avg_optical_flow = (1.0f - alpha) * avg_optical_flow + alpha * frame_metrics.optical_flow_time_ms;
    avg_transform_calc = (1.0f - alpha) * avg_transform_calc + alpha * frame_metrics.transform_calc_time_ms;
    avg_smoothing = (1.0f - alpha) * avg_smoothing + alpha * frame_metrics.smoothing_time_ms;
    
    // Update current metrics with averages
    current_metrics_.feature_detection_time_ms = avg_feature_detection;
    current_metrics_.optical_flow_time_ms = avg_optical_flow;
    current_metrics_.transform_calc_time_ms = avg_transform_calc;
    current_metrics_.smoothing_time_ms = avg_smoothing;
    
    // Log detailed breakdown at configurable intervals based on framerate
    static int log_interval = 300; // Default: 10 seconds at 30fps
    if (frame_count % log_interval == 0) {
        log_performance_breakdown();
        
        // Adaptive interval based on processing time
        if (frame_metrics.processing_time_ms > 16.0f) { // >16ms suggests <60fps
            log_interval = 150; // 5 seconds at 30fps
        } else {
            log_interval = 600; // 10 seconds at 60fps
        }
    }
}

void StabilizerCore::log_performance_breakdown() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Performance Breakdown: ";
    ss << "Feature=" << current_metrics_.feature_detection_time_ms << "ms ";
    ss << "Flow=" << current_metrics_.optical_flow_time_ms << "ms ";
    ss << "Transform=" << current_metrics_.transform_calc_time_ms << "ms ";
    ss << "Smooth=" << current_metrics_.smoothing_time_ms << "ms ";
    ss << "Total=" << current_metrics_.processing_time_ms << "ms ";
    ss << "Features=" << current_metrics_.tracked_features;
    ss << " Success=" << (current_metrics_.tracking_success_rate * 100.0f) << "%";
    
    STABILIZER_LOG_INFO( "%s", ss.str().c_str());
}

#else

// Stub implementations for non-OpenCV builds
void StabilizerCore::update_detailed_metrics(const StabilizerMetrics&) {
    // No-op in stub mode
}

void StabilizerCore::log_performance_breakdown() const {
    STABILIZER_LOG_INFO( "Performance breakdown: stub mode - no detailed metrics available");
}

#endif

} // namespace obs_stabilizer
/*
OBS Stabilizer Plugin - OBS Studio Integration Layer Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "obs_integration.hpp"
#include <util/bmem.h>
#include <algorithm>

namespace obs_stabilizer {

// StabilizerFilter Implementation

StabilizerFilter::StabilizerFilter() : source(nullptr), enabled(true) {
    stabilizer_core = std::make_unique<StabilizerCore>();
    
    // Initialize with default configuration
    config = StabilizerConfig{};
    stabilizer_core->initialize(config);
}

StabilizerFilter::~StabilizerFilter() {
    if (stabilizer_core) {
        stabilizer_core->reset();
    }
}

void StabilizerFilter::update_settings(obs_data_t* settings) {
    enabled = obs_data_get_bool(settings, "enable_stabilization");
    
    // Core stabilization parameters
    config.smoothing_radius = std::max(10, std::min(100, (int)obs_data_get_int(settings, "smoothing_radius")));
    config.max_features = std::max(100, std::min(1000, (int)obs_data_get_int(settings, "max_features")));
    config.error_threshold = std::max(10.0f, std::min(100.0f, (float)obs_data_get_double(settings, "error_threshold")));
    config.enable_stabilization = enabled;
    
    // Preset system
    int preset_value = (int)obs_data_get_int(settings, "preset_mode");
    if (preset_value >= 0 && preset_value <= 3) {
        auto preset_mode = static_cast<StabilizerConfig::PresetMode>(preset_value);
        if (preset_mode != StabilizerConfig::PresetMode::CUSTOM) {
            apply_preset_configuration(preset_mode);
        }
    }
    
    // Output mode configuration
    int output_value = (int)obs_data_get_int(settings, "output_mode");
    if (output_value >= 0 && output_value <= 2) {
        config.output_mode = static_cast<StabilizerConfig::OutputMode>(output_value);
    }
    
    // Advanced parameters
    config.min_feature_quality = std::max(0.001f, std::min(0.1f, (float)obs_data_get_double(settings, "min_feature_quality")));
    config.refresh_threshold = std::max(10, std::min(50, (int)obs_data_get_int(settings, "refresh_threshold")));
    config.adaptive_refresh = obs_data_get_bool(settings, "adaptive_refresh");
    
    // Performance settings
    config.enable_gpu_acceleration = obs_data_get_bool(settings, "enable_gpu_acceleration");
    config.processing_threads = std::max(1, std::min(8, (int)obs_data_get_int(settings, "processing_threads")));
    
    // Update core with new configuration
    if (stabilizer_core) {
        stabilizer_core->update_configuration(config);
    }
    
    obs_log(LOG_INFO, "Stabilizer settings updated: enabled=%s, smoothing=%d, features=%d, threshold=%.1f",
            enabled ? "true" : "false", config.smoothing_radius, config.max_features, config.error_threshold);
}

void StabilizerFilter::apply_preset_configuration(StabilizerConfig::PresetMode preset) {
    switch (preset) {
        case StabilizerConfig::PresetMode::GAMING:
            // Gaming preset: Fast response, lower quality
            config.smoothing_radius = 15;
            config.max_features = 150;
            config.error_threshold = 40.0f;
            config.output_mode = StabilizerConfig::OutputMode::CROP;
            config.min_feature_quality = 0.02f;
            config.refresh_threshold = 20;
            break;
            
        case StabilizerConfig::PresetMode::STREAMING:
            // Streaming preset: Balanced performance and quality
            config.smoothing_radius = 30;
            config.max_features = 200;
            config.error_threshold = 30.0f;
            config.output_mode = StabilizerConfig::OutputMode::PAD;
            config.min_feature_quality = 0.01f;
            config.refresh_threshold = 25;
            break;
            
        case StabilizerConfig::PresetMode::RECORDING:
            // Recording preset: High quality, slower response
            config.smoothing_radius = 50;
            config.max_features = 400;
            config.error_threshold = 20.0f;
            config.output_mode = StabilizerConfig::OutputMode::SCALE_FIT;
            config.min_feature_quality = 0.005f;
            config.refresh_threshold = 30;
            break;
            
        case StabilizerConfig::PresetMode::CUSTOM:
        default:
            // Custom mode: Don't change settings
            break;
    }
    
    config.preset = preset;
}

void StabilizerFilter::set_default_settings(obs_data_t* settings) {
    // Main toggle
    obs_data_set_default_bool(settings, "enable_stabilization", true);
    
    // Preset system (default to Streaming)
    obs_data_set_default_int(settings, "preset_mode", (int)StabilizerConfig::PresetMode::STREAMING);
    
    // Core parameters (Streaming preset defaults)
    obs_data_set_default_int(settings, "smoothing_radius", 30);
    obs_data_set_default_int(settings, "max_features", 200);
    obs_data_set_default_double(settings, "error_threshold", 30.0);
    
    // Output mode
    obs_data_set_default_int(settings, "output_mode", (int)StabilizerConfig::OutputMode::PAD);
    
    // Advanced parameters
    obs_data_set_default_double(settings, "min_feature_quality", 0.01);
    obs_data_set_default_int(settings, "refresh_threshold", 25);
    obs_data_set_default_bool(settings, "adaptive_refresh", true);
    
    // Performance settings
    obs_data_set_default_bool(settings, "enable_gpu_acceleration", false);
    obs_data_set_default_int(settings, "processing_threads", 1);
}

// OBSIntegration Implementation

bool OBSIntegration::plugin_load() {
    // Register the stabilizer filter
    struct obs_source_info stabilizer_filter_info = {};
    stabilizer_filter_info.id = "stabilizer_filter";
    stabilizer_filter_info.type = OBS_SOURCE_TYPE_FILTER;
    stabilizer_filter_info.output_flags = OBS_SOURCE_VIDEO;
    stabilizer_filter_info.get_name = filter_get_name;
    stabilizer_filter_info.create = filter_create;
    stabilizer_filter_info.destroy = filter_destroy;
    stabilizer_filter_info.filter_video = filter_video;
    stabilizer_filter_info.update = filter_update;
    stabilizer_filter_info.get_properties = filter_properties;
    stabilizer_filter_info.get_defaults = filter_defaults;
    
    obs_register_source(&stabilizer_filter_info);
    
    obs_log(LOG_INFO, "OBS Stabilizer Plugin loaded successfully");
    return true;
}

void OBSIntegration::plugin_unload() {
    obs_log(LOG_INFO, "OBS Stabilizer Plugin unloaded");
}

const char* OBSIntegration::filter_get_name(void* unused) {
    UNUSED_PARAMETER(unused);
    return obs_module_text("Stabilizer");
}

void* OBSIntegration::filter_create(obs_data_t* settings, obs_source_t* source) {
    StabilizerFilter* filter = new StabilizerFilter();
    
    filter->source = source;
    filter->update_settings(settings);
    
    obs_log(LOG_INFO, "Stabilizer filter created");
    return filter;
}

void OBSIntegration::filter_destroy(void* data) {
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    
    if (filter) {
        obs_log(LOG_INFO, "Stabilizer filter destroyed");
        delete filter;
    }
}

struct obs_source_frame* OBSIntegration::filter_video(void* data, struct obs_source_frame* frame) {
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    
    if (!filter || !filter->enabled || !filter->stabilizer_core) {
        return frame; // Pass through if disabled or invalid
    }
    
    // Validate frame data
    if (!validate_frame_data(frame)) {
        return frame;
    }
    
#ifdef ENABLE_STABILIZATION
    // Process frame through stabilization core
    TransformResult result = filter->stabilizer_core->process_frame(frame);
    
    if (result.success && !result.transform_matrix.empty()) {
        // Validate transformation matrix
        if (validate_transform_matrix(result.transform_matrix)) {
            // Apply transformation to frame
            apply_transform_to_frame(frame, result.transform_matrix);
        } else {
            obs_log(LOG_WARNING, "Invalid transformation matrix, skipping frame transform");
        }
    }
#endif
    
    return frame;
}

void OBSIntegration::filter_update(void* data, obs_data_t* settings) {
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    
    if (filter) {
        filter->update_settings(settings);
    }
}

obs_properties_t* OBSIntegration::filter_properties(void* data) {
    UNUSED_PARAMETER(data);
    
    obs_properties_t* props = obs_properties_create();
    
    // Main enable/disable toggle (top-level)
    obs_properties_add_bool(props, "enable_stabilization", obs_module_text("Enable Video Stabilization"));
    
    // Preset selection (primary control)
    obs_property_t* preset_list = obs_properties_add_list(props, "preset_mode",
                                                         obs_module_text("Stabilization Preset"), 
                                                         OBS_COMBO_TYPE_LIST, 
                                                         OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(preset_list, obs_module_text("Custom"), (int)StabilizerConfig::PresetMode::CUSTOM);
    obs_property_list_add_int(preset_list, obs_module_text("Gaming (Fast Response)"), (int)StabilizerConfig::PresetMode::GAMING);
    obs_property_list_add_int(preset_list, obs_module_text("Streaming (Balanced)"), (int)StabilizerConfig::PresetMode::STREAMING);
    obs_property_list_add_int(preset_list, obs_module_text("Recording (High Quality)"), (int)StabilizerConfig::PresetMode::RECORDING);
    obs_property_set_long_description(preset_list,
        obs_module_text("Choose a preset optimized for your use case, or select Custom for manual configuration."));
    
    // Core parameter group (visible when Custom selected or for reference)
    obs_property_t* core_group = obs_properties_create_group(props, "core_params", 
                                                            obs_module_text("Stabilization Parameters"), 
                                                            OBS_GROUP_NORMAL);
    
    // Smoothing strength slider (10-100)
    obs_property_t* smoothing_prop = obs_properties_add_int_slider(
        core_group, "smoothing_radius", obs_module_text("Smoothing Strength"), 10, 100, 5);
    obs_property_set_long_description(smoothing_prop, 
        obs_module_text("Number of frames used for transform smoothing. Higher values = smoother but more latency."));
    
    // Feature points slider (100-1000)
    obs_property_t* features_prop = obs_properties_add_int_slider(
        core_group, "max_features", obs_module_text("Feature Points"), 100, 1000, 50);
    obs_property_set_long_description(features_prop,
        obs_module_text("Maximum number of feature points to track. Higher values = more accurate but slower."));
    
    // Stability threshold slider (10.0-100.0)
    obs_property_t* threshold_prop = obs_properties_add_float_slider(
        core_group, "error_threshold", obs_module_text("Stability Threshold"), 10.0, 100.0, 5.0);
    obs_property_set_long_description(threshold_prop,
        obs_module_text("Error threshold for tracking quality. Lower values = stricter quality requirements."));
    
    // Output mode selection
    obs_property_t* output_mode = obs_properties_add_list(core_group, "output_mode",
                                                         obs_module_text("Edge Handling"), 
                                                         OBS_COMBO_TYPE_LIST, 
                                                         OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(output_mode, obs_module_text("Crop Borders"), (int)StabilizerConfig::OutputMode::CROP);
    obs_property_list_add_int(output_mode, obs_module_text("Black Padding"), (int)StabilizerConfig::OutputMode::PAD);
    obs_property_list_add_int(output_mode, obs_module_text("Scale to Fit"), (int)StabilizerConfig::OutputMode::SCALE_FIT);
    obs_property_set_long_description(output_mode,
        obs_module_text("How to handle stabilization borders: Crop removes edges, Padding adds black borders, Scale stretches to fit."));
    
    // Advanced settings (collapsible group)
    obs_property_t* advanced_group = obs_properties_create_group(props, "advanced_params",
                                                                obs_module_text("Advanced Settings"),
                                                                OBS_GROUP_CHECKABLE);
    
    // Feature quality threshold
    obs_property_t* quality_prop = obs_properties_add_float_slider(
        advanced_group, "min_feature_quality", obs_module_text("Feature Quality"), 0.001, 0.1, 0.001);
    obs_property_set_long_description(quality_prop,
        obs_module_text("Minimum quality threshold for feature detection. Lower values detect more features but may be less stable."));
    
    // Refresh threshold
    obs_property_t* refresh_prop = obs_properties_add_int_slider(
        advanced_group, "refresh_threshold", obs_module_text("Refresh Threshold"), 10, 50, 5);
    obs_property_set_long_description(refresh_prop,
        obs_module_text("Number of frames before refreshing feature detection. Lower values = more responsive but higher CPU usage."));
    
    // Adaptive refresh toggle
    obs_property_t* adaptive_prop = obs_properties_add_bool(advanced_group, "adaptive_refresh", obs_module_text("Adaptive Refresh"));
    obs_property_set_long_description(adaptive_prop,
        obs_module_text("Automatically adjust refresh rate based on tracking quality."));
    
    // GPU acceleration toggle (experimental)
    obs_property_t* gpu_prop = obs_properties_add_bool(advanced_group, "enable_gpu_acceleration", obs_module_text("GPU Acceleration (Experimental)"));
    obs_property_set_long_description(gpu_prop,
        obs_module_text("Enable GPU acceleration for stabilization processing. May not be available on all systems."));
    
    // Processing threads
    obs_property_t* threads_prop = obs_properties_add_int_slider(
        advanced_group, "processing_threads", obs_module_text("Processing Threads"), 1, 8, 1);
    obs_property_set_long_description(threads_prop,
        obs_module_text("Number of threads to use for stabilization processing. Higher values may improve performance on multi-core systems."));
    
    return props;
}

void OBSIntegration::filter_defaults(obs_data_t* settings) {
    StabilizerFilter filter;
    filter.set_default_settings(settings);
}

void OBSIntegration::apply_transform_to_frame(struct obs_source_frame* frame, const cv::Mat& transform) {
#ifdef ENABLE_STABILIZATION
    // Route to format-specific transformation
    switch (frame->format) {
        case VIDEO_FORMAT_NV12:
            apply_transform_nv12(frame, transform);
            break;
        case VIDEO_FORMAT_I420:
            apply_transform_i420(frame, transform);
            break;
        default:
            obs_log(LOG_WARNING, "Unsupported video format for transformation: %d", frame->format);
            break;
    }
#endif
}

#ifdef ENABLE_STABILIZATION
void OBSIntegration::apply_transform_nv12(struct obs_source_frame* frame, const cv::Mat& transform) {
    try {
        // Validate frame data
        if (!frame->data[0] || frame->linesize[0] < frame->width) {
            obs_log(LOG_ERROR, "Invalid NV12 Y plane data or linesize");
            return;
        }
        
        // Create OpenCV Mat for Y plane
        cv::Mat y_plane(frame->height, frame->width, CV_8UC1, frame->data[0], frame->linesize[0]);
        cv::Mat y_transformed;
        
        // Apply transformation to Y plane
        cv::warpAffine(y_plane, y_transformed, transform, 
                      cv::Size(frame->width, frame->height), 
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));
        
        // Copy transformed Y plane back
        y_transformed.copyTo(y_plane);
        
        // For UV plane, apply transformation at half resolution
        // Validate NV12 UV plane access
        if (!frame->data[1] || frame->linesize[1] < frame->width) {
            obs_log(LOG_ERROR, "Invalid NV12 UV plane data or linesize");
            return;
        }
        cv::Mat uv_plane(frame->height/2, frame->width/2, CV_8UC2,
                        frame->data[1], frame->linesize[1]);
        cv::Mat uv_transformed;
        
        // Scale transform for half-resolution UV plane
        cv::Mat uv_transform = transform.clone();
        uv_transform.at<double>(0, 2) /= 2.0; // Scale translation X
        uv_transform.at<double>(1, 2) /= 2.0; // Scale translation Y
        
        cv::warpAffine(uv_plane, uv_transformed, uv_transform,
                      cv::Size(frame->width/2, frame->height/2),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128, 128));
        
        // Copy transformed UV plane back
        uv_transformed.copyTo(uv_plane);
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "NV12 transformation failed: %s", e.what());
    }
}

void OBSIntegration::apply_transform_i420(struct obs_source_frame* frame, const cv::Mat& transform) {
    try {
        // Validate Y plane access
        if (!frame->data[0] || frame->linesize[0] < frame->width) {
            obs_log(LOG_ERROR, "Invalid I420 Y plane data or linesize");
            return;
        }
        
        // Transform Y plane
        cv::Mat y_plane(frame->height, frame->width, CV_8UC1, frame->data[0], frame->linesize[0]);
        cv::Mat y_transformed;
        
        cv::warpAffine(y_plane, y_transformed, transform,
                      cv::Size(frame->width, frame->height),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));
        y_transformed.copyTo(y_plane);
        
        // Scale transform for half-resolution chroma planes  
        cv::Mat chroma_transform = transform.clone();
        chroma_transform.at<double>(0, 2) /= 2.0; // Scale translation X
        chroma_transform.at<double>(1, 2) /= 2.0; // Scale translation Y
        
        // Validate U plane access
        if (!frame->data[1] || frame->linesize[1] < frame->width/2) {
            obs_log(LOG_ERROR, "Invalid I420 U plane data or linesize");
            return;
        }
        
        // Transform U plane
        cv::Mat u_plane(frame->height/2, frame->width/2, CV_8UC1, frame->data[1], frame->linesize[1]);
        cv::Mat u_transformed;
        
        cv::warpAffine(u_plane, u_transformed, chroma_transform,
                      cv::Size(frame->width/2, frame->height/2),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128));
        u_transformed.copyTo(u_plane);
        
        // Validate V plane access
        if (!frame->data[2] || frame->linesize[2] < frame->width/2) {
            obs_log(LOG_ERROR, "Invalid I420 V plane data or linesize");
            return;
        }
        
        // Transform V plane
        cv::Mat v_plane(frame->height/2, frame->width/2, CV_8UC1, frame->data[2], frame->linesize[2]);
        cv::Mat v_transformed;
        
        cv::warpAffine(v_plane, v_transformed, chroma_transform,
                      cv::Size(frame->width/2, frame->height/2),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128));
        v_transformed.copyTo(v_plane);
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "I420 transformation failed: %s", e.what());
    }
}
#endif

bool OBSIntegration::validate_frame_data(struct obs_source_frame* frame) {
    // Comprehensive input validation
    if (!frame || !frame->data[0] || frame->width == 0 || frame->height == 0) {
        obs_log(LOG_ERROR, "Invalid frame data for stabilization");
        return false;
    }
    
    // Validate frame dimensions and prevent integer overflow
    if (frame->width > 8192 || frame->height > 8192) {
        obs_log(LOG_ERROR, "Frame dimensions too large: %ux%u", frame->width, frame->height);
        return false;
    }
    
    // Validate that width*height won't overflow
    if ((size_t)frame->width * frame->height > SIZE_MAX / 4) {
        obs_log(LOG_ERROR, "Frame size would cause integer overflow");
        return false;
    }
    
    return true;
}

bool OBSIntegration::validate_transform_matrix(const cv::Mat& transform) {
#ifdef ENABLE_STABILIZATION
    if (transform.empty() || transform.rows != 2 || transform.cols != 3) {
        return false;
    }
    
    // Check for reasonable transformation values
    double dx = transform.at<double>(0, 2);
    double dy = transform.at<double>(1, 2);
    double scale = sqrt(transform.at<double>(0, 0) * transform.at<double>(0, 0) + 
                       transform.at<double>(0, 1) * transform.at<double>(0, 1));
    
    // Reject unreasonable transformations
    if (abs(dx) > 200 || abs(dy) > 200 || scale < 0.1 || scale > 3.0) {
        obs_log(LOG_WARNING, "Rejecting unreasonable transform: dx=%.2f, dy=%.2f, scale=%.2f", 
                dx, dy, scale);
        return false;
    }
    
    return true;
#else
    UNUSED_PARAMETER(transform);
    return false;
#endif
}

} // namespace obs_stabilizer
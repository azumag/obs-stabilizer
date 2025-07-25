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
    enabled = obs_data_get_bool(settings, "enabled");
    
    // Validate and clamp settings to safe ranges
    config.smoothing_radius = std::max(10, std::min(100, (int)obs_data_get_int(settings, "smoothing_radius")));
    config.max_features = std::max(100, std::min(1000, (int)obs_data_get_int(settings, "max_features")));
    config.enable_stabilization = enabled;
    
    // Update core with new configuration
    if (stabilizer_core) {
        stabilizer_core->update_configuration(config);
    }
    
    obs_log(LOG_INFO, "Stabilizer settings updated: enabled=%s, smoothing=%d, features=%d",
            enabled ? "true" : "false", config.smoothing_radius, config.max_features);
}

void StabilizerFilter::set_default_settings(obs_data_t* settings) {
    obs_data_set_default_bool(settings, "enabled", true);
    obs_data_set_default_int(settings, "smoothing_radius", 30);
    obs_data_set_default_int(settings, "max_features", 200);
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
    
    // Enable/disable stabilization
    obs_properties_add_bool(props, "enabled", obs_module_text("Enable Stabilization"));
    
    // Smoothing radius (10-100)
    obs_property_t* smoothing_prop = obs_properties_add_int_slider(
        props, "smoothing_radius", obs_module_text("Smoothing Radius"), 10, 100, 5);
    obs_property_set_long_description(smoothing_prop, 
        obs_module_text("Number of frames used for transform smoothing. Higher values = smoother but more latency."));
    
    // Maximum feature points (100-1000)
    obs_property_t* features_prop = obs_properties_add_int_slider(
        props, "max_features", obs_module_text("Max Feature Points"), 100, 1000, 50);
    obs_property_set_long_description(features_prop,
        obs_module_text("Maximum number of feature points to track. Higher values = more accurate but slower."));
    
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
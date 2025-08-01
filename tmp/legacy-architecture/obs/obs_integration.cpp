/*
OBS Stabilizer Plugin - OBS Studio Integration Layer Implementation
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "obs_integration.hpp"
#include "../core/error_handler.hpp"
#include "../core/parameter_validator.hpp"
#include "../core/stabilizer_constants.hpp"
#include "../core/ui_constants.hpp"
#include <algorithm>
#include <memory>
#include <cstdlib>

namespace obs_stabilizer {

static bool validate_filter_data_integrity(void* data) {
    if (!data) return false;

    // Check pointer alignment (required for safe access)
    uintptr_t addr = reinterpret_cast<uintptr_t>(data);
    if (addr % sizeof(void*) != 0) {
        return false;
    }

    // Basic address space validation - avoid obviously invalid pointers
    // This is a conservative check for user-space addresses
    if (addr < 0x1000 || addr > 0x7FFFFFFFFFFFULL) {
        return false;
    }

    // For production safety, we'll use a simple approach:
    // Validate that the pointer size is reasonable and the memory
    // appears to be in a valid region without complex signal handling

    try {
        // Attempt to safely validate the structure
        // We'll check if the first few bytes look like a valid structure
        StabilizerFilter* potential_filter = static_cast<StabilizerFilter*>(data);

        // Check if stabilizer_core appears to be a valid unique_ptr
        // unique_ptr should never be null in a properly constructed StabilizerFilter
        if (!potential_filter->stabilizer_core) {
            return false;
        }

        // Validate that the core pointer appears reasonable
        const void* core_ptr = potential_filter->stabilizer_core.get();
        if (!core_ptr) {
            return false;
        }

        // Additional validation: check if core pointer is in reasonable address space
        uintptr_t core_addr = reinterpret_cast<uintptr_t>(core_ptr);
        if (core_addr < 0x1000 || core_addr > 0x7FFFFFFFFFFFULL) {
            return false;
        }

        // If we reach here, the structure appears to have valid invariants
        return true;

    } catch (const std::bad_alloc&) {
        return false; // Memory issues
    } catch (const std::exception&) {
        return false; // Any other standard exception
    } catch (...) {
        return false; // Catch any other exceptions (should not happen with proper code)
    }
}

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
    config.smoothing_radius = std::max(StabilizerConstants::MIN_SMOOTHING_RADIUS,
                                       std::min(StabilizerConstants::MAX_SMOOTHING_RADIUS,
                                               (int)obs_data_get_int(settings, "smoothing_radius")));
    config.max_features = std::max(StabilizerConstants::UIRanges::MIN_UI_FEATURES,
                                   std::min(StabilizerConstants::UIRanges::MAX_UI_FEATURES,
                                           (int)obs_data_get_int(settings, "max_features")));
    config.error_threshold = std::max(static_cast<float>(StabilizerConstants::UIRanges::MIN_UI_THRESHOLD),
                                      std::min(static_cast<float>(StabilizerConstants::UIRanges::MAX_UI_THRESHOLD),
                                              (float)obs_data_get_double(settings, "error_threshold")));
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
            config.smoothing_radius = StabilizerConstants::Presets::GAMING_SMOOTHING;
            config.max_features = StabilizerConstants::Presets::GAMING_FEATURES;
            config.error_threshold = 40.0f;
            config.output_mode = StabilizerConfig::OutputMode::CROP;
            config.min_feature_quality = 0.02f;
            config.refresh_threshold = 20;
            break;

        case StabilizerConfig::PresetMode::STREAMING:
            // Streaming preset: Balanced performance and quality
            config.smoothing_radius = StabilizerConstants::Presets::STREAMING_SMOOTHING;
            config.max_features = StabilizerConstants::Presets::STREAMING_FEATURES;
            config.error_threshold = static_cast<float>(StabilizerConstants::Presets::STREAMING_THRESHOLD);
            config.output_mode = StabilizerConfig::OutputMode::PAD;
            config.min_feature_quality = 0.01f;
            config.refresh_threshold = 25;
            break;

        case StabilizerConfig::PresetMode::RECORDING:
            // Recording preset: High quality, slower response
            config.smoothing_radius = StabilizerConstants::Presets::RECORDING_SMOOTHING;
            config.max_features = StabilizerConstants::Presets::RECORDING_FEATURES;
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
    obs_data_set_default_int(settings, "smoothing_radius", StabilizerConstants::DEFAULT_SMOOTHING_RADIUS);
    obs_data_set_default_int(settings, "max_features", StabilizerConstants::Presets::STREAMING_FEATURES);
    obs_data_set_default_double(settings, "error_threshold", StabilizerConstants::Presets::STREAMING_THRESHOLD);

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
    return UIConstants::Strings::FILTER_NAME;
}

void* OBSIntegration::filter_create(obs_data_t* settings, obs_source_t* source) {
    void* result = nullptr;

    ErrorHandler::safe_execute([&]() {
        // Use unique_ptr for exception safety during construction
        auto filter = std::make_unique<StabilizerFilter>();

        filter->source = source;
        filter->update_settings(settings);

        obs_log(LOG_INFO, "Stabilizer filter created");

        // Release ownership to OBS - OBS will manage lifetime via filter_destroy
        result = filter.release();
    }, ErrorCategory::INITIALIZATION, "filter_create");

    return result;
}

void OBSIntegration::filter_destroy(void* data) {
    if (!data) {
        obs_log(LOG_WARNING, "Attempted to destroy null stabilizer filter");
        return;
    }

    ErrorHandler::safe_execute([&]() {
        // Type-safe validation before casting
        if (!validate_filter_data_integrity(data)) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "filter_destroy", "Invalid filter data integrity");
            return;
        }

        StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
        obs_log(LOG_INFO, "Stabilizer filter destroyed");

        // Use unique_ptr for RAII cleanup - ensures destructor is called even if exceptions occur
        std::unique_ptr<StabilizerFilter> filter_guard(filter);

        // filter_guard will automatically delete filter when going out of scope
    }, ErrorCategory::CLEANUP, "filter_destroy");
}

struct obs_source_frame* OBSIntegration::filter_video(void* data, struct obs_source_frame* frame) {
    struct obs_source_frame* result_frame = frame;
    ErrorHandler::safe_execute([&]() {
        // Type-safe validation before casting
        if (!validate_filter_data_integrity(data)) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "filter_video", "Invalid filter data integrity");
            return; // Keep result_frame as original frame
        }

        StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);

        if (!filter || !filter->enabled || !filter->stabilizer_core) {
            return; // Pass through if disabled or invalid
        }

        // Validate frame data
        auto frame_validation = ParameterValidator::validate_frame_basic(frame);
        if (!frame_validation) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "filter_video", frame_validation.error_message);
            return;
        }

#ifdef ENABLE_STABILIZATION
        // Process frame through stabilization core
        TransformResult result = filter->stabilizer_core->process_frame(frame);

        if (result.success && !result.transform_matrix.is_empty()) {
            // Validate transformation matrix
            if (validate_transform_matrix(result.transform_matrix)) {
                // Apply transformation to frame
                apply_transform_to_frame(frame, result.transform_matrix);
            } else {
                ErrorHandler::log_warning(ErrorCategory::VALIDATION, "filter_video",
                                      "Invalid transformation matrix, skipping frame transform");
            }
        }
#endif
    }, ErrorCategory::FRAME_PROCESSING, "filter_video");

    return result_frame;
}

void OBSIntegration::filter_update(void* data, obs_data_t* settings) {
    ErrorHandler::safe_execute([&]() {
        // Type-safe validation before casting
        if (!validate_filter_data_integrity(data)) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "filter_update", "Invalid filter data integrity");
            return;
        }

        StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);

        if (filter) {
            filter->update_settings(settings);
        }
    }, ErrorCategory::CONFIGURATION, "filter_update");
}

obs_properties_t* OBSIntegration::filter_properties(void* data) {
    UNUSED_PARAMETER(data);

    obs_properties_t* props = obs_properties_create();

    // Main enable/disable toggle (top-level)
    obs_properties_add_bool(props, "enable_stabilization", UIConstants::Strings::ENABLE_STABILIZATION_TEXT);

    // Preset selection (primary control)
    obs_property_t* preset_list = obs_properties_add_list(props, "preset_mode",
                                                         UIConstants::Strings::STABILIZATION_PRESET,
                                                         OBS_COMBO_TYPE_LIST,
                                                         OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(preset_list, UIConstants::Strings::CUSTOM, (int)StabilizerConfig::PresetMode::CUSTOM);
    obs_property_list_add_int(preset_list, UIConstants::Strings::GAMING_PRESET, (int)StabilizerConfig::PresetMode::GAMING);
    obs_property_list_add_int(preset_list, UIConstants::Strings::STREAMING_PRESET, (int)StabilizerConfig::PresetMode::STREAMING);
    obs_property_list_add_int(preset_list, UIConstants::Strings::RECORDING_PRESET, (int)StabilizerConfig::PresetMode::RECORDING);
    obs_property_set_long_description(preset_list,
        UIConstants::Strings::PRESET_DESCRIPTION);

    // Core parameter group (visible when Custom selected or for reference)
    // Smoothing strength slider
    obs_property_t* smoothing_prop = obs_properties_add_int_slider(
        props, "smoothing_radius", UIConstants::Strings::SMOOTHING_STRENGTH,
        StabilizerConstants::UIRanges::MIN_UI_SMOOTHING,
        StabilizerConstants::UIRanges::MAX_UI_SMOOTHING, 5);
    obs_property_set_long_description(smoothing_prop,
        UIConstants::Strings::SMOOTHING_DESCRIPTION);

    // Feature points slider
    obs_property_t* features_prop = obs_properties_add_int_slider(
        props, "max_features", UIConstants::Strings::FEATURE_POINTS,
        StabilizerConstants::UIRanges::MIN_UI_FEATURES,
        StabilizerConstants::UIRanges::MAX_UI_FEATURES, 50);
    obs_property_set_long_description(features_prop,
        UIConstants::Strings::FEATURE_POINTS_DESCRIPTION);

    // Stability threshold slider
    obs_property_t* threshold_prop = obs_properties_add_float_slider(
        props, "error_threshold", UIConstants::Strings::STABILITY_THRESHOLD,
        StabilizerConstants::UIRanges::MIN_UI_THRESHOLD,
        StabilizerConstants::UIRanges::MAX_UI_THRESHOLD, 5.0);
    obs_property_set_long_description(threshold_prop,
        UIConstants::Strings::STABILITY_DESCRIPTION);

    // Output mode selection
    obs_property_t* output_mode = obs_properties_add_list(props, "output_mode",
                                                         UIConstants::Strings::EDGE_HANDLING,
                                                         OBS_COMBO_TYPE_LIST,
                                                         OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(output_mode, UIConstants::Strings::CROP_BORDERS, (int)StabilizerConfig::OutputMode::CROP);
    obs_property_list_add_int(output_mode, UIConstants::Strings::BLACK_PADDING, (int)StabilizerConfig::OutputMode::PAD);
    obs_property_list_add_int(output_mode, UIConstants::Strings::SCALE_TO_FIT, (int)StabilizerConfig::OutputMode::SCALE_FIT);
    obs_property_set_long_description(output_mode,
        UIConstants::Strings::EDGE_DESCRIPTION);

    // Feature quality threshold
    obs_property_t* quality_prop = obs_properties_add_float_slider(
        props, "min_feature_quality", UIConstants::Strings::FEATURE_QUALITY, 0.001, 0.1, 0.001);
    obs_property_set_long_description(quality_prop,
        UIConstants::Strings::FEATURE_QUALITY_DESCRIPTION);

    // Refresh threshold
    obs_property_t* refresh_prop = obs_properties_add_int_slider(
        props, "refresh_threshold", UIConstants::Strings::REFRESH_THRESHOLD, 10, 50, 5);
    obs_property_set_long_description(refresh_prop,
        UIConstants::Strings::REFRESH_DESCRIPTION);

    // Adaptive refresh toggle
    obs_property_t* adaptive_prop = obs_properties_add_bool(props, "adaptive_refresh", UIConstants::Strings::ADAPTIVE_REFRESH);
    obs_property_set_long_description(adaptive_prop,
        UIConstants::Strings::ADAPTIVE_DESCRIPTION);

    // GPU acceleration toggle (experimental)
    obs_property_t* gpu_prop = obs_properties_add_bool(props, "enable_gpu_acceleration", UIConstants::Strings::GPU_ACCELERATION);
    obs_property_set_long_description(gpu_prop,
        UIConstants::Strings::GPU_DESCRIPTION);

    // Processing threads
    obs_property_t* threads_prop = obs_properties_add_int_slider(
        props, "processing_threads", UIConstants::Strings::PROCESSING_THREADS, 1, 8, 1);
    obs_property_set_long_description(threads_prop,
        UIConstants::Strings::THREADS_DESCRIPTION);

    return props;
}

void OBSIntegration::filter_defaults(obs_data_t* settings) {
    StabilizerFilter filter;
    filter.set_default_settings(settings);
}

void OBSIntegration::apply_transform_to_frame(struct obs_source_frame* frame, const TransformMatrix& transform) {
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
            ErrorHandler::log_warning(ErrorCategory::VALIDATION, "apply_transform_to_frame",
                                      "Unsupported video format for transformation");
            break;
    }
#endif
}

#ifdef ENABLE_STABILIZATION
void OBSIntegration::apply_transform_nv12(struct obs_source_frame* frame, const TransformMatrix& transform) {
    ErrorHandler::safe_execute([&]() {
        // Validate frame data
        if (!frame->data[0] || frame->linesize[0] < frame->width) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "apply_transform_nv12",
                                   "Invalid NV12 Y plane data or linesize");
            return;
        }

        // Convert TransformMatrix to cv::Mat for OpenCV operations
        cv::Mat cv_transform = transform.to_opencv_mat();

        // Create OpenCV Mat for Y plane
        cv::Mat y_plane(frame->height, frame->width, CV_8UC1, frame->data[0], frame->linesize[0]);
        cv::Mat y_transformed;

        // Apply transformation to Y plane
        cv::warpAffine(y_plane, y_transformed, cv_transform,
                      cv::Size(frame->width, frame->height),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));

        // Copy transformed Y plane back
        y_transformed.copyTo(y_plane);

        // For UV plane, apply transformation at half resolution
        // Validate NV12 UV plane access
        if (!frame->data[1] || frame->linesize[1] < frame->width) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "apply_transform_nv12",
                                   "Invalid NV12 UV plane data or linesize");
            return;
        }
        cv::Mat uv_plane(frame->height/2, frame->width/2, CV_8UC2,
                        frame->data[1], frame->linesize[1]);
        cv::Mat uv_transformed;

        // Scale transform for half-resolution UV plane
        cv::Mat uv_transform = cv_transform.clone();
        uv_transform.at<double>(0, 2) /= 2.0; // Scale translation X
        uv_transform.at<double>(1, 2) /= 2.0; // Scale translation Y

        cv::warpAffine(uv_plane, uv_transformed, uv_transform,
                      cv::Size(frame->width/2, frame->height/2),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128, 128));

        // Copy transformed UV plane back
        uv_transformed.copyTo(uv_plane);
    }, ErrorCategory::FRAME_PROCESSING, "apply_transform_nv12");
}

void OBSIntegration::apply_transform_i420(struct obs_source_frame* frame, const TransformMatrix& transform) {
    ErrorHandler::safe_execute([&]() {
        // Validate Y plane access using unified validation
        auto validation_result = ParameterValidator::validate_frame_i420(frame);
        if (!validation_result) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "apply_transform_i420",
                                   validation_result.error_message);
            return;
        }

        // Convert TransformMatrix to cv::Mat for OpenCV operations
        cv::Mat cv_transform = transform.to_opencv_mat();

        // Transform Y plane
        cv::Mat y_plane(frame->height, frame->width, CV_8UC1, frame->data[0], frame->linesize[0]);
        cv::Mat y_transformed;

        cv::warpAffine(y_plane, y_transformed, cv_transform,
                      cv::Size(frame->width, frame->height),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));
        y_transformed.copyTo(y_plane);

        // Scale transform for half-resolution chroma planes
        cv::Mat chroma_transform = cv_transform.clone();
        chroma_transform.at<double>(0, 2) /= 2.0; // Scale translation X
        chroma_transform.at<double>(1, 2) /= 2.0; // Scale translation Y

        // Validate U plane access
        if (!frame->data[1] || frame->linesize[1] < frame->width/2) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "apply_transform_i420",
                                   "Invalid I420 U plane data or linesize");
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
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "apply_transform_i420",
                                   "Invalid I420 V plane data or linesize");
            return;
        }

        // Transform V plane
        cv::Mat v_plane(frame->height/2, frame->width/2, CV_8UC1, frame->data[2], frame->linesize[2]);
        cv::Mat v_transformed;

        cv::warpAffine(v_plane, v_transformed, chroma_transform,
                      cv::Size(frame->width/2, frame->height/2),
                      cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128));
        v_transformed.copyTo(v_plane);

    }, ErrorCategory::FRAME_PROCESSING, "apply_transform_i420");
}

template<typename PlaneProcessor>
void OBSIntegration::apply_transform_generic(struct obs_source_frame* frame,
                                           const TransformMatrix& transform,
                                           PlaneProcessor process_planes) {
    ErrorHandler::safe_execute([&]() {
        // Validate frame data
        auto frame_validation = ParameterValidator::validate_frame_basic(frame);
        if (!frame_validation) {
            ErrorHandler::log_error(ErrorCategory::VALIDATION, "apply_transform_to_frame", frame_validation.error_message);
            return;
        }

        // Convert TransformMatrix to cv::Mat for OpenCV operations
        cv::Mat cv_transform = transform.to_opencv_mat();

        // Process all planes using the provided processor
        process_planes(frame, cv_transform);

    }, ErrorCategory::FRAME_PROCESSING, "apply_transform_generic");
}

#endif


bool OBSIntegration::validate_transform_matrix(const TransformMatrix& transform) {
#ifdef ENABLE_STABILIZATION
    if (transform.is_empty() || !transform.is_valid()) {
        return false;
    }

    // Check for reasonable transformation values using TransformMatrix interface
    double dx = transform.get_translation_x();
    double dy = transform.get_translation_y();
    double scale = transform.get_scale();

    // Reject unreasonable transformations
    if (std::abs(dx) > StabilizerConstants::MAX_TRANSLATION_WARN ||
        std::abs(dy) > StabilizerConstants::MAX_TRANSLATION_WARN ||
        scale < StabilizerConstants::MIN_SCALE_FACTOR ||
        scale > StabilizerConstants::MAX_SCALE_FACTOR) {
        ErrorHandler::log_warning(ErrorCategory::VALIDATION, "validate_transform_matrix",
                                  "Rejecting unreasonable transform values");
        return false;
    }

    return true;
#else
    UNUSED_PARAMETER(transform);
    return false;
#endif
}

} // namespace obs_stabilizer
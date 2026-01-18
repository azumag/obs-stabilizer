/*
 * OBS Stabilizer Plugin - Refactored with Modular Architecture
 * Uses the new modular design with StabilizerCore and OBSIntegration layers
 * Maintains compatibility with existing OBS API structure
 */

#include <obs-module.h>
#include "core/stabilizer_core.hpp"
#include <memory>
#include <cstring>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <algorithm>

// OBS module declarations - using existing macros from stub headers

// Plugin filter data structure using the new modular architecture
struct stabilizer_filter {
    obs_source_t *source;
    std::unique_ptr<StabilizerCore> stabilizer;
    bool initialized;
    StabilizerCore::StabilizerParams params;
    
    // Performance monitoring
    uint64_t frame_count;
    double avg_processing_time;
};

// Forward declarations
static const char *stabilizer_filter_name(void *unused);
static const char *stabilizer_filter_id(void *unused);
static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source);
static void stabilizer_filter_destroy(void *data);
static void stabilizer_filter_update(void *data, obs_data_t *settings);
static obs_source_frame *stabilizer_filter_video(void *data, obs_source_frame *frame);
static obs_properties_t *stabilizer_filter_properties(void *data);
static void stabilizer_filter_get_defaults(obs_data_t *settings);

// Preset callback function
static bool preset_changed_callback(void *priv, obs_properties_t *props, obs_property_t *property, 
                                   obs_data_t *settings);
static void apply_preset(obs_data_t *settings, const char *preset_name);

// Parameter conversion functions
static StabilizerCore::StabilizerParams settings_to_params(const obs_data_t *settings);
static void params_to_settings(const StabilizerCore::StabilizerParams& params, obs_data_t *settings);

// Frame conversion functions
static cv::Mat obs_frame_to_cv_mat(const obs_source_frame *frame);
static obs_source_frame *cv_mat_to_obs_frame(const cv::Mat& mat, const obs_source_frame *reference_frame);

// Plugin structure definition
static struct obs_source_info stabilizer_filter_info = {
    .id = "stabilizer_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = stabilizer_filter_name,
    .create = stabilizer_filter_create,
    .destroy = stabilizer_filter_destroy,
    .update = stabilizer_filter_update,
    .video_render = NULL,
    .filter_video = stabilizer_filter_video,
    .get_properties = stabilizer_filter_properties,
    .get_defaults = stabilizer_filter_get_defaults,
};

// Plugin implementation functions

static const char *stabilizer_filter_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "Video Stabilizer";
}

static const char *stabilizer_filter_id(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "stabilizer_filter";
}

static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source)
{
    try {
        struct stabilizer_filter *context = new stabilizer_filter();
        if (!context) {
            obs_log(LOG_ERROR, "Failed to allocate filter context");
            return nullptr;
        }

        context->source = source;
        context->stabilizer = std::make_unique<StabilizerCore>();
        context->initialized = false;
        context->frame_count = 0;
        context->avg_processing_time = 0.0;

        // Get initial parameters
        context->params = settings_to_params(settings);
        
        // Validate parameters
        if (!StabilizerCore::validate_parameters(context->params)) {
            obs_log(LOG_ERROR, "Invalid parameters provided during filter creation");
            delete context;
            return nullptr;
        }

        obs_log(LOG_INFO, "Stabilizer filter created successfully");
        return context;

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in filter create: %s", e.what());
        return nullptr;
    }
}

static void stabilizer_filter_destroy(void *data)
{
    try {
        struct stabilizer_filter *context = (struct stabilizer_filter *)data;
        if (context) {
            delete context;
        }
        obs_log(LOG_INFO, "Stabilizer filter destroyed");

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in filter destroy: %s", e.what());
    }
}

static void stabilizer_filter_update(void *data, obs_data_t *settings)
{
    try {
        struct stabilizer_filter *context = (struct stabilizer_filter *)data;
        if (!context || !context->stabilizer) {
            obs_log(LOG_ERROR, "Invalid context in filter update");
            return;
        }

        StabilizerCore::StabilizerParams new_params = settings_to_params(settings);
        
        if (StabilizerCore::validate_parameters(new_params)) {
            context->params = new_params;
            if (context->initialized) {
                context->stabilizer->update_parameters(new_params);
            }
        } else {
            obs_log(LOG_ERROR, "Invalid parameters in filter update");
        }

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in filter update: %s", e.what());
    }
}

static obs_source_frame *stabilizer_filter_video(void *data, obs_source_frame *frame)
{
    try {
        struct stabilizer_filter *context = (struct stabilizer_filter *)data;
        if (!context || !context->stabilizer || !frame) {
            return frame;
        }

        // Initialize stabilizer on first frame
        if (!context->initialized) {
            if (!context->stabilizer->initialize(frame->width, frame->height, context->params)) {
                obs_log(LOG_ERROR, "Failed to initialize stabilizer: %s", 
                        context->stabilizer->get_last_error().c_str());
                return frame;
            }
            
            context->initialized = true;
            obs_log(LOG_INFO, "Stabilizer initialized for %dx%d", frame->width, frame->height);
        }

        // Convert OBS frame to OpenCV Mat
        cv::Mat cv_frame = obs_frame_to_cv_mat(frame);
        if (cv_frame.empty()) {
            obs_log(LOG_ERROR, "Failed to convert OBS frame to OpenCV Mat");
            return frame;
        }

        // Process frame with stabilizer
        auto start_time = std::chrono::high_resolution_clock::now();
        cv::Mat stabilized_frame = context->stabilizer->process_frame(cv_frame);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        // Update performance metrics
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time = duration.count() / 1000.0;
        context->frame_count++;
        context->avg_processing_time = (context->avg_processing_time * (context->frame_count - 1) + processing_time) / context->frame_count;

        // Convert back to OBS frame
        obs_source_frame* result = cv_mat_to_obs_frame(stabilized_frame, frame);
        
        return result ? result : frame;

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in video processing: %s", e.what());
        return frame;
    }
}

static obs_properties_t *stabilizer_filter_properties(void *data)
{
    try {
        obs_properties_t *props = obs_properties_create();
        
        // Basic properties
        obs_properties_add_bool(props, "enabled", "Enable Stabilization");
        
        // Preset selector
        obs_property_t* preset_list = obs_properties_add_list(props, "preset", "Preset", 
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
        obs_property_list_add_string(preset_list, "Gaming", "gaming");
        obs_property_list_add_string(preset_list, "Streaming", "streaming");
        obs_property_list_add_string(preset_list, "Recording", "recording");
        obs_property_list_add_string(preset_list, "Custom", "custom");
        
        obs_property_set_modified_callback(preset_list, preset_changed_callback);
        
        // Basic parameters
        obs_properties_add_int_slider(props, "smoothing_radius", "Smoothing Radius", 5, 200, 1);
        obs_properties_add_float_slider(props, "max_correction", "Max Correction (%)", 1.0, 100.0, 0.5);
        obs_properties_add_int_slider(props, "feature_count", "Feature Count", 50, 2000, 10);
        
        // Advanced parameters
        obs_properties_add_float_slider(props, "quality_level", "Quality Level", 0.001, 0.1, 0.001);
        obs_properties_add_float_slider(props, "min_distance", "Min Distance", 1.0, 200.0, 1.0);
        obs_properties_add_int_slider(props, "block_size", "Block Size", 3, 31, 2);
        
        obs_properties_add_bool(props, "use_harris", "Use Harris Detector");
        obs_properties_add_float_slider(props, "k", "Harris K Parameter", 0.01, 0.1, 0.001);
        
        // Debug options
        obs_properties_add_bool(props, "debug_mode", "Debug Mode");
        
        return props;

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in get properties: %s", e.what());
        return obs_properties_create();
    }
}

static void stabilizer_filter_get_defaults(obs_data_t *settings)
{
    try {
        // Use streaming preset as default
        StabilizerCore::StabilizerParams default_params = StabilizerCore::get_preset_streaming();
        params_to_settings(default_params, settings);
        
        obs_data_set_default_string(settings, "preset", "streaming");

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in get defaults: %s", e.what());
    }
}

// Preset callback function
static bool preset_changed_callback(void *priv, obs_properties_t *props, obs_property_t *property, 
                                   obs_data_t *settings)
{
    UNUSED_PARAMETER(priv);
    UNUSED_PARAMETER(props);
    UNUSED_PARAMETER(property);
    
    const char* preset = obs_data_get_string(settings, "preset");
    if (!preset || strlen(preset) == 0) {
        return true;
    }
    
    if (strcmp(preset, "custom") != 0) {
        apply_preset(settings, preset);
    }
    
    return true;
}

static void apply_preset(obs_data_t *settings, const char *preset_name)
{
    StabilizerCore::StabilizerParams params;
    
    if (strcmp(preset_name, "gaming") == 0) {
        params = StabilizerCore::get_preset_gaming();
    } else if (strcmp(preset_name, "streaming") == 0) {
        params = StabilizerCore::get_preset_streaming();
    } else if (strcmp(preset_name, "recording") == 0) {
        params = StabilizerCore::get_preset_recording();
    } else {
        // Custom preset - don't change values
        return;
    }
    
    params_to_settings(params, settings);
}

// Parameter conversion functions
static StabilizerCore::StabilizerParams settings_to_params(const obs_data_t *settings)
{
    StabilizerCore::StabilizerParams params;
    
    // Direct parameter access with defaults - OBS API functions don't throw exceptions
    // Use safe defaults if keys don't exist
    params.enabled = obs_data_get_bool(settings, "enabled");
    params.smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
    params.max_correction = (float)obs_data_get_double(settings, "max_correction");
    params.feature_count = (int)obs_data_get_int(settings, "feature_count");
    params.quality_level = (float)obs_data_get_double(settings, "quality_level");
    params.min_distance = (float)obs_data_get_double(settings, "min_distance");
    params.block_size = (int)obs_data_get_int(settings, "block_size");
    params.use_harris = obs_data_get_bool(settings, "use_harris");
    params.k = (float)obs_data_get_double(settings, "k");
    params.debug_mode = obs_data_get_bool(settings, "debug_mode");
    
    // Validate and clamp parameters
    params.smoothing_radius = std::clamp(params.smoothing_radius, 5, 200);
    params.max_correction = std::clamp(params.max_correction, 1.0f, 100.0f);
    params.feature_count = std::clamp(params.feature_count, 50, 2000);
    params.quality_level = std::clamp(params.quality_level, 0.001f, 0.1f);
    params.min_distance = std::clamp(params.min_distance, 1.0f, 200.0f);
    params.block_size = std::clamp(params.block_size, 3, 31);
    if (params.block_size % 2 == 0) params.block_size++; // Ensure odd number
    params.k = std::clamp(params.k, 0.01f, 0.1f);
    
    return params;
}

static void params_to_settings(const StabilizerCore::StabilizerParams& params, obs_data_t *settings)
{
    obs_data_set_bool(settings, "enabled", params.enabled);
    obs_data_set_int(settings, "smoothing_radius", params.smoothing_radius);
    obs_data_set_double(settings, "max_correction", params.max_correction);
    obs_data_set_int(settings, "feature_count", params.feature_count);
    obs_data_set_double(settings, "quality_level", params.quality_level);
    obs_data_set_double(settings, "min_distance", params.min_distance);
    obs_data_set_int(settings, "block_size", params.block_size);
    obs_data_set_bool(settings, "use_harris", params.use_harris);
    obs_data_set_double(settings, "k", params.k);
    obs_data_set_bool(settings, "debug_mode", params.debug_mode);
}

// Frame conversion functions
static cv::Mat obs_frame_to_cv_mat(const obs_source_frame *frame)
{
    if (!frame || !frame->data[0]) {
        return cv::Mat();
    }
    
    try {
        cv::Mat mat;
        
        switch (frame->format) {
            case VIDEO_FORMAT_BGRA:
                mat = cv::Mat(frame->height, frame->width, CV_8UC4, frame->data[0], frame->linesize[0]);
                break;
                
            case VIDEO_FORMAT_BGRX:
                mat = cv::Mat(frame->height, frame->width, CV_8UC4, frame->data[0], frame->linesize[0]);
                break;
                
            case VIDEO_FORMAT_BGR3:
                mat = cv::Mat(frame->height, frame->width, CV_8UC3, frame->data[0], frame->linesize[0]);
                break;
                
            case VIDEO_FORMAT_NV12:
                // Convert NV12 to BGRA
                {
                    cv::Mat yuv(frame->height + frame->height/2, frame->width, CV_8UC1, frame->data[0]);
                    cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGRA_NV12);
                }
                break;
                
            case VIDEO_FORMAT_I420:
                // Convert I420 to BGRA
                {
                    cv::Mat yuv(frame->height + frame->height/2, frame->width, CV_8UC1, frame->data[0]);
                    cv::cvtColor(yuv, mat, cv::COLOR_YUV2BGRA_I420);
                }
                break;
                
            default:
                obs_log(LOG_ERROR, "Unsupported frame format: %d", frame->format);
                return cv::Mat();
        }
        
        return mat.clone();
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "OpenCV exception in obs_frame_to_cv_mat: %s", e.what());
        return cv::Mat();
    }
}

// Frame buffer for safe frame modification (static to persist between calls)
// This buffer ensures we never modify the reference frame in-place.
// Instead, we create a complete copy of the frame data and return a pointer to our internal buffer.
static struct {
    std::vector<uint8_t> buffer;
    obs_source_frame frame;
    bool initialized;
} frame_buffer = { {}, {}, false };

/**
 * Converts an OpenCV Mat to an OBS source frame.
 * 
 * This function creates a complete copy of the frame data and does NOT modify the reference frame.
 * The returned frame uses our internal frame_buffer and is safe for the caller to use.
 * 
 * @param mat The OpenCV matrix to convert
 * @param reference_frame The reference frame to copy metadata from
 * @return Pointer to our internal frame buffer with the converted data, or nullptr on error
 */
static obs_source_frame *cv_mat_to_obs_frame(const cv::Mat& mat, const obs_source_frame *reference_frame)
{
    if (mat.empty() || !reference_frame) {
        return nullptr;
    }
    
    try {
        // Initialize frame buffer if needed
        if (!frame_buffer.initialized) {
            frame_buffer.frame = *reference_frame;  // Copy structure
            frame_buffer.initialized = true;
        }
        
        // Update frame properties from reference
        frame_buffer.frame.width = reference_frame->width;
        frame_buffer.frame.height = reference_frame->height;
        frame_buffer.frame.format = reference_frame->format;
        frame_buffer.frame.timestamp = reference_frame->timestamp;
        frame_buffer.frame.flip = reference_frame->flip;
        
        // Copy color matrix and range data
        memcpy(frame_buffer.frame.color_matrix, reference_frame->color_matrix, 
               sizeof(reference_frame->color_matrix));
        frame_buffer.frame.full_range = reference_frame->full_range;
        memcpy(frame_buffer.frame.color_range_min, reference_frame->color_range_min,
               sizeof(reference_frame->color_range_min));
        memcpy(frame_buffer.frame.color_range_max, reference_frame->color_range_max,
               sizeof(reference_frame->color_range_max));
        
        // Convert mat to the appropriate format
        cv::Mat converted;
        
        switch (reference_frame->format) {
            case VIDEO_FORMAT_BGRA:
                if (mat.channels() == 4) {
                    converted = mat.clone();
                } else if (mat.channels() == 3) {
                    cv::cvtColor(mat, converted, cv::COLOR_BGR2BGRA);
                } else {
                    cv::cvtColor(mat, converted, cv::COLOR_GRAY2BGRA);
                }
                break;
                
            case VIDEO_FORMAT_BGR3:
                if (mat.channels() == 3) {
                    converted = mat.clone();
                } else if (mat.channels() == 4) {
                    cv::cvtColor(mat, converted, cv::COLOR_BGRA2BGR);
                } else {
                    cv::cvtColor(mat, converted, cv::COLOR_GRAY2BGR);
                }
                break;
                
            default:
                obs_log(LOG_ERROR, "Unsupported output format: %d", reference_frame->format);
                return nullptr;
        }
        
        // Calculate required buffer size
        size_t required_size = converted.total() * converted.elemSize();
        
        // Reallocate buffer if needed
        if (frame_buffer.buffer.size() < required_size) {
            frame_buffer.buffer.resize(required_size);
        }
        
        // Update frame data pointers to use our buffer
        frame_buffer.frame.data[0] = frame_buffer.buffer.data();
        frame_buffer.frame.linesize[0] = static_cast<uint32_t>(converted.step);
        
        // Copy converted data to buffer
        memcpy(frame_buffer.frame.data[0], converted.data, required_size);
        
        // Clear other data planes
        for (int i = 1; i < 8; i++) {
            frame_buffer.frame.data[i] = nullptr;
            frame_buffer.frame.linesize[i] = 0;
        }
        
        return &frame_buffer.frame;
        
    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in cv_mat_to_obs_frame: %s", e.what());
        return nullptr;
    }
}

// Plugin entry points
bool obs_module_load(void)
{
    obs_log(LOG_INFO, "Loading OBS Stabilizer Plugin (Modular Architecture)");
    
    // Register the filter
    if (!obs_register_source(&stabilizer_filter_info)) {
        obs_log(LOG_ERROR, "Failed to register stabilizer filter");
        return false;
    }
    
    obs_log(LOG_INFO, "OBS Stabilizer Plugin loaded successfully");
    return true;
}

void obs_module_unload(void)
{
    obs_log(LOG_INFO, "OBS Stabilizer Plugin unloaded");
}

// Module exports
MODULE_EXPORT const char *obs_module_name(void)
{
    return "obs-stabilizer";
}

MODULE_EXPORT const char *obs_module_author(void)
{
    return "OBS Stabilizer Team";
}

MODULE_EXPORT const char *obs_module_version(void)
{
    return "0.2.0";
}

MODULE_EXPORT void obs_module_set_pointer(obs_module_t *module)
{
    (void)module;
}
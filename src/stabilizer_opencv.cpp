/*
 * OBS Stabilizer Plugin - Refactored with Modular Architecture
 * Uses the new modular design with StabilizerCore and OBSIntegration layers
 * Maintains compatibility with existing OBS API structure
 */

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>
#include "core/frame_utils.hpp"
#endif

#include "core/stabilizer_core.hpp"
#include "core/stabilizer_wrapper.hpp"
#include "core/adaptive_stabilizer.hpp"
#include <memory>
#include <cstring>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <algorithm>

// OBS module declarations - using existing macros from stub headers

#ifdef HAVE_OBS_HEADERS
// Plugin filter data structure using the new modular architecture with RAII wrapper
struct stabilizer_filter {
    obs_source_t *source;
    StabilizerWrapper stabilizer;  // Using RAII wrapper for memory safety
    bool initialized;
    StabilizerCore::StabilizerParams params;
    
    // Performance monitoring
    uint64_t frame_count;
    double avg_processing_time;
    
    // Adaptive stabilization support
    bool adaptive_enabled;
    double motion_sensitivity;
    double transition_rate;
    AdaptiveStabilization::AdaptiveConfig adaptive_config;
    std::unique_ptr<AdaptiveStabilization::AdaptiveStabilizer> adaptive_stabilizer;
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
#endif

// Plugin implementation functions

#ifdef HAVE_OBS_HEADERS
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
        auto context = std::make_unique<struct stabilizer_filter>();

        context->source = source;
        context->initialized = false;
        context->frame_count = 0;
        context->avg_processing_time = 0.0;

        // Get initial parameters
        context->params = settings_to_params(settings);
        
        // Get adaptive settings
        context->adaptive_enabled = obs_data_get_bool(settings, "adaptive_enabled");
        context->motion_sensitivity = obs_data_get_double(settings, "motion_sensitivity");
        context->transition_rate = obs_data_get_double(settings, "transition_rate");
        
        // Set adaptive config from settings
        context->adaptive_config.static_smoothing = obs_data_get_int(settings, "static_smoothing");
        context->adaptive_config.static_correction = obs_data_get_double(settings, "static_correction");
        context->adaptive_config.static_features = obs_data_get_int(settings, "static_features");
        context->adaptive_config.static_quality = obs_data_get_double(settings, "static_quality");
        
        context->adaptive_config.slow_smoothing = obs_data_get_int(settings, "slow_smoothing");
        context->adaptive_config.slow_correction = obs_data_get_double(settings, "slow_correction");
        context->adaptive_config.slow_features = obs_data_get_int(settings, "slow_features");
        context->adaptive_config.slow_quality = obs_data_get_double(settings, "slow_quality");
        
        context->adaptive_config.fast_smoothing = obs_data_get_int(settings, "fast_smoothing");
        context->adaptive_config.fast_correction = obs_data_get_double(settings, "fast_correction");
        context->adaptive_config.fast_features = obs_data_get_int(settings, "fast_features");
        context->adaptive_config.fast_quality = obs_data_get_double(settings, "fast_quality");
        
        context->adaptive_config.shake_smoothing = obs_data_get_int(settings, "shake_smoothing");
        context->adaptive_config.shake_correction = obs_data_get_double(settings, "shake_correction");
        context->adaptive_config.shake_features = obs_data_get_int(settings, "shake_features");
        context->adaptive_config.shake_quality = obs_data_get_double(settings, "shake_quality");
        
        context->adaptive_config.pan_smoothing = obs_data_get_int(settings, "pan_smoothing");
        context->adaptive_config.pan_correction = obs_data_get_double(settings, "pan_correction");
        context->adaptive_config.pan_features = obs_data_get_int(settings, "pan_features");
        context->adaptive_config.pan_quality = obs_data_get_double(settings, "pan_quality");
        
        context->adaptive_config.transition_rate = context->transition_rate;
        
        // Create adaptive stabilizer if enabled
        if (context->adaptive_enabled) {
            context->adaptive_stabilizer = std::make_unique<AdaptiveStabilization::AdaptiveStabilizer>(context->adaptive_config);
        }
        
        // Validate parameters
        if (!StabilizerCore::validate_parameters(context->params)) {
            obs_log(LOG_ERROR, "Invalid parameters provided during filter creation");
            return nullptr;
        }

        obs_log(LOG_INFO, "Stabilizer filter created successfully");
        return context.release();

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in filter create: %s", e.what());
        return nullptr;
    }
}

static void stabilizer_filter_destroy(void *data)
{
    try {
        // Use RAII pattern for safe memory management
        auto context = std::unique_ptr<struct stabilizer_filter>(
            static_cast<struct stabilizer_filter *>(data)
        );
        // RAII automatically handles cleanup when context goes out of scope
        obs_log(LOG_INFO, "Stabilizer filter destroyed");

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in filter destroy: %s", e.what());
    }
}

static void stabilizer_filter_update(void *data, obs_data_t *settings)
{
    try {
        struct stabilizer_filter *context = (struct stabilizer_filter *)data;
        if (!context) {
            obs_log(LOG_ERROR, "Invalid context in filter update");
            return;
        }

        StabilizerCore::StabilizerParams new_params = settings_to_params(settings);
        
        if (StabilizerCore::validate_parameters(new_params)) {
            context->params = new_params;
            if (context->initialized) {
                // Re-initialize with new parameters
                uint32_t width = obs_source_get_width(context->source);
                uint32_t height = obs_source_get_height(context->source);
                if (width > 0 && height > 0) {
                    context->stabilizer.initialize(width, height, new_params);
                }
            }
        } else {
            obs_log(LOG_ERROR, "Invalid parameters in filter update");
        }
        
        // Update adaptive settings
        bool new_adaptive_enabled = obs_data_get_bool(settings, "adaptive_enabled");
        double new_motion_sensitivity = obs_data_get_double(settings, "motion_sensitivity");
        double new_transition_rate = obs_data_get_double(settings, "transition_rate");
        
        // Recreate adaptive stabilizer if enabled/disabled state changes
        if (new_adaptive_enabled && !context->adaptive_enabled) {
            context->adaptive_stabilizer = std::make_unique<AdaptiveStabilization::AdaptiveStabilizer>(context->adaptive_config);
            context->adaptive_enabled = true;
            obs_log(LOG_INFO, "Adaptive stabilizer enabled");
        } else if (!new_adaptive_enabled && context->adaptive_enabled) {
            context->adaptive_stabilizer.reset();
            context->adaptive_enabled = false;
            obs_log(LOG_INFO, "Adaptive stabilizer disabled");
        }
        
        // Update adaptive parameters
        context->motion_sensitivity = new_motion_sensitivity;
        context->transition_rate = new_transition_rate;
        
        context->adaptive_config.static_smoothing = obs_data_get_int(settings, "static_smoothing");
        context->adaptive_config.static_correction = obs_data_get_double(settings, "static_correction");
        context->adaptive_config.static_features = obs_data_get_int(settings, "static_features");
        context->adaptive_config.static_quality = obs_data_get_double(settings, "static_quality");
        
        context->adaptive_config.slow_smoothing = obs_data_get_int(settings, "slow_smoothing");
        context->adaptive_config.slow_correction = obs_data_get_double(settings, "slow_correction");
        context->adaptive_config.slow_features = obs_data_get_int(settings, "slow_features");
        context->adaptive_config.slow_quality = obs_data_get_double(settings, "slow_quality");
        
        context->adaptive_config.fast_smoothing = obs_data_get_int(settings, "fast_smoothing");
        context->adaptive_config.fast_correction = obs_data_get_double(settings, "fast_correction");
        context->adaptive_config.fast_features = obs_data_get_int(settings, "fast_features");
        context->adaptive_config.fast_quality = obs_data_get_double(settings, "fast_quality");
        
        context->adaptive_config.shake_smoothing = obs_data_get_int(settings, "shake_smoothing");
        context->adaptive_config.shake_correction = obs_data_get_double(settings, "shake_correction");
        context->adaptive_config.shake_features = obs_data_get_int(settings, "shake_features");
        context->adaptive_config.shake_quality = obs_data_get_double(settings, "shake_quality");
        
        context->adaptive_config.pan_smoothing = obs_data_get_int(settings, "pan_smoothing");
        context->adaptive_config.pan_correction = obs_data_get_double(settings, "pan_correction");
        context->adaptive_config.pan_features = obs_data_get_int(settings, "pan_features");
        context->adaptive_config.pan_quality = obs_data_get_double(settings, "pan_quality");
        
        context->adaptive_config.transition_rate = new_transition_rate;
        
        // Update adaptive stabilizer if exists
        if (context->adaptive_stabilizer) {
            context->adaptive_stabilizer->set_config(context->adaptive_config);
            context->adaptive_stabilizer->set_motion_sensitivity(new_motion_sensitivity);
        }

    } catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Exception in filter update: %s", e.what());
    }
}

static obs_source_frame *stabilizer_filter_video(void *data, obs_source_frame *frame)
{
    try {
        struct stabilizer_filter *context = (struct stabilizer_filter *)data;
        if (!context || !context->stabilizer.is_initialized() || !frame) {
            return frame;
        }

        // Initialize stabilizer on first frame
        if (!context->initialized) {
            if (!context->stabilizer.initialize(frame->width, frame->height, context->params)) {
                obs_log(LOG_ERROR, "Failed to initialize stabilizer: %s", 
                         context->stabilizer.get_last_error().c_str());
                return frame;
            }
            
            // Initialize adaptive stabilizer if enabled
            if (context->adaptive_enabled && context->adaptive_stabilizer) {
                if (!context->adaptive_stabilizer->initialize(frame->width, frame->height, context->params)) {
                    obs_log(LOG_ERROR, "Failed to initialize adaptive stabilizer: %s", 
                             context->adaptive_stabilizer->get_last_error().c_str());
                } else {
                    obs_log(LOG_INFO, "Adaptive stabilizer initialized for %dx%d", frame->width, frame->height);
                }
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
        
        cv::Mat stabilized_frame;
        if (context->adaptive_enabled && context->adaptive_stabilizer && 
            context->adaptive_stabilizer->is_ready()) {
            // Use adaptive stabilizer
            stabilized_frame = context->adaptive_stabilizer->process_frame(cv_frame);
        } else {
            // Use standard stabilizer
            stabilized_frame = context->stabilizer.process_frame(cv_frame);
        }
        
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
        
        // Adaptive stabilization toggle
        obs_properties_add_bool(props, "adaptive_enabled", "Enable Adaptive Stabilization");
        
        // Adaptive settings (only visible when adaptive enabled)
        obs_properties_t* adaptive_group = obs_properties_create();
        
        obs_properties_add_float_slider(adaptive_group, "motion_sensitivity", "Motion Sensitivity", 0.5, 2.0, 0.1);
        obs_properties_add_float_slider(adaptive_group, "transition_rate", "Parameter Transition Rate", 0.1, 1.0, 0.1);
        
        // Motion-specific parameters
        obs_properties_t* static_group = obs_properties_create();
        obs_properties_add_int_slider(static_group, "static_smoothing", "Static Smoothing Radius", 5, 100, 1);
        obs_properties_add_float_slider(static_group, "static_correction", "Static Max Correction (%)", 1.0, 50.0, 0.5);
        obs_properties_add_int_slider(static_group, "static_features", "Static Feature Count", 50, 500, 10);
        obs_properties_add_float_slider(static_group, "static_quality", "Static Quality Level", 0.001, 0.1, 0.001);
        
        obs_properties_t* slow_group = obs_properties_create();
        obs_properties_add_int_slider(slow_group, "slow_smoothing", "Slow Smoothing Radius", 5, 100, 1);
        obs_properties_add_float_slider(slow_group, "slow_correction", "Slow Max Correction (%)", 1.0, 50.0, 0.5);
        obs_properties_add_int_slider(slow_group, "slow_features", "Slow Feature Count", 50, 500, 10);
        obs_properties_add_float_slider(slow_group, "slow_quality", "Slow Quality Level", 0.001, 0.1, 0.001);
        
        obs_properties_t* fast_group = obs_properties_create();
        obs_properties_add_int_slider(fast_group, "fast_smoothing", "Fast Smoothing Radius", 5, 100, 1);
        obs_properties_add_float_slider(fast_group, "fast_correction", "Fast Max Correction (%)", 1.0, 50.0, 0.5);
        obs_properties_add_int_slider(fast_group, "fast_features", "Fast Feature Count", 50, 500, 10);
        obs_properties_add_float_slider(fast_group, "fast_quality", "Fast Quality Level", 0.001, 0.1, 0.001);
        
        obs_properties_t* shake_group = obs_properties_create();
        obs_properties_add_int_slider(shake_group, "shake_smoothing", "Shake Smoothing Radius", 5, 100, 1);
        obs_properties_add_float_slider(shake_group, "shake_correction", "Shake Max Correction (%)", 1.0, 50.0, 0.5);
        obs_properties_add_int_slider(shake_group, "shake_features", "Shake Feature Count", 50, 500, 10);
        obs_properties_add_float_slider(shake_group, "shake_quality", "Shake Quality Level", 0.001, 0.1, 0.001);
        
        obs_properties_t* pan_group = obs_properties_create();
        obs_properties_add_int_slider(pan_group, "pan_smoothing", "Pan/Zoom Smoothing Radius", 5, 100, 1);
        obs_properties_add_float_slider(pan_group, "pan_correction", "Pan/Zoom Max Correction (%)", 1.0, 50.0, 0.5);
        obs_properties_add_int_slider(pan_group, "pan_features", "Pan/Zoom Feature Count", 50, 500, 10);
        obs_properties_add_float_slider(pan_group, "pan_quality", "Pan/Zoom Quality Level", 0.001, 0.1, 0.001);
        
        // Add groups to main properties
        obs_properties_add_group(props, adaptive_group, "Adaptive Settings", OBS_GROUP_NORMAL, true);
        obs_properties_add_group(props, static_group, "Static Parameters", OBS_GROUP_NORMAL, true);
        obs_properties_add_group(props, slow_group, "Slow Motion Parameters", OBS_GROUP_NORMAL, true);
        obs_properties_add_group(props, fast_group, "Fast Motion Parameters", OBS_GROUP_NORMAL, true);
        obs_properties_add_group(props, shake_group, "Camera Shake Parameters", OBS_GROUP_NORMAL, true);
        obs_properties_add_group(props, pan_group, "Pan/Zoom Parameters", OBS_GROUP_NORMAL, true);
        
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
        
        // Adaptive defaults (disabled by default for backward compatibility)
        obs_data_set_default_bool(settings, "adaptive_enabled", false);
        obs_data_set_default_double(settings, "motion_sensitivity", 1.0);
        obs_data_set_default_double(settings, "transition_rate", 0.5);
        
        // Default adaptive config
        obs_data_set_default_int(settings, "static_smoothing", 8);
        obs_data_set_default_double(settings, "static_correction", 15.0);
        obs_data_set_default_int(settings, "static_features", 120);
        obs_data_set_default_double(settings, "static_quality", 0.015);
        
        obs_data_set_default_int(settings, "slow_smoothing", 25);
        obs_data_set_default_double(settings, "slow_correction", 25.0);
        obs_data_set_default_int(settings, "slow_features", 175);
        obs_data_set_default_double(settings, "slow_quality", 0.010);
        
        obs_data_set_default_int(settings, "fast_smoothing", 50);
        obs_data_set_default_double(settings, "fast_correction", 35.0);
        obs_data_set_default_int(settings, "fast_features", 250);
        obs_data_set_default_double(settings, "fast_quality", 0.010);
        
        obs_data_set_default_int(settings, "shake_smoothing", 65);
        obs_data_set_default_double(settings, "shake_correction", 45.0);
        obs_data_set_default_int(settings, "shake_features", 350);
        obs_data_set_default_double(settings, "shake_quality", 0.005);
        
        obs_data_set_default_int(settings, "pan_smoothing", 15);
        obs_data_set_default_double(settings, "pan_correction", 20.0);
        obs_data_set_default_int(settings, "pan_features", 225);
        obs_data_set_default_double(settings, "pan_quality", 0.010);

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
    params.enabled = obs_data_get_bool(const_cast<obs_data_t*>(settings), "enabled");
    params.smoothing_radius = (int)obs_data_get_int(const_cast<obs_data_t*>(settings), "smoothing_radius");
    params.max_correction = (float)obs_data_get_double(const_cast<obs_data_t*>(settings), "max_correction");
    params.feature_count = (int)obs_data_get_int(const_cast<obs_data_t*>(settings), "feature_count");
    params.quality_level = (float)obs_data_get_double(const_cast<obs_data_t*>(settings), "quality_level");
    params.min_distance = (float)obs_data_get_double(const_cast<obs_data_t*>(settings), "min_distance");
    params.block_size = (int)obs_data_get_int(const_cast<obs_data_t*>(settings), "block_size");
    params.use_harris = obs_data_get_bool(const_cast<obs_data_t*>(settings), "use_harris");
    params.k = (float)obs_data_get_double(const_cast<obs_data_t*>(settings), "k");
    params.debug_mode = obs_data_get_bool(const_cast<obs_data_t*>(settings), "debug_mode");
    
    // Use centralized parameter validation for consistency
    params = VALIDATION::validate_parameters(params);
    
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

// Frame conversion functions using centralized utilities
static cv::Mat obs_frame_to_cv_mat(const obs_source_frame *frame)
{
    if (!frame || !frame->data[0]) {
        return cv::Mat();
    }
    
    try {
        // Use centralized frame conversion utility
        return FRAME_UTILS::Conversion::obs_to_cv(frame);
        
    } catch (const cv::Exception& e) {
        obs_log(LOG_ERROR, "OpenCV exception in obs_frame_to_cv_mat: %s", e.what());
        return cv::Mat();
    }
}

// Frame buffer for safe frame modification (static to persist between calls)
// This buffer ensures we never modify the reference frame in-place.
// Instead, we create a complete copy of the frame data and return a pointer to our internal buffer.
static std::mutex frame_buffer_mutex;
static struct {
    std::vector<uint8_t> buffer;
    obs_source_frame frame;
    bool initialized;
} frame_buffer = { {}, {}, false };

/**
 * Converts an OpenCV Mat to an OBS source frame using centralized utilities.
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
        // Use centralized frame conversion utility with thread-safe buffer management
        return FRAME_UTILS::FrameBuffer::create(mat, reference_frame);
        
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
#endif // HAVE_OBS_HEADERS

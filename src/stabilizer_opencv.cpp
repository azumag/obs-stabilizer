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
#include "core/parameter_validation.hpp"
#include <memory>
#include <cstring>
#include <opencv2/opencv.hpp>
#include <chrono>

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
};

// Forward declarations
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
        // Note: Parameters are already validated via VALIDATION::validate_parameters in settings_to_params()
        context->params = settings_to_params(settings);

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

        // Note: settings_to_params() already calls VALIDATION::validate_parameters() at line 346
        StabilizerCore::StabilizerParams new_params = settings_to_params(settings);

        // Direct assignment - validation already done in settings_to_params()
        context->params = new_params;

        if (context->initialized) {
            // Re-initialize with new parameters
            uint32_t width = obs_source_get_width(context->source);
            uint32_t height = obs_source_get_height(context->source);
            if (width > 0 && height > 0) {
                context->stabilizer.initialize(width, height, new_params);
            }
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

        cv::Mat stabilized_frame = context->stabilizer.process_frame(cv_frame);

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

        // Edge handling (Issue #226)
        obs_property_t* edge_mode = obs_properties_add_list(props, "edge_handling",
                        "Edge Handling",
                        OBS_COMBO_TYPE_LIST,
                        OBS_COMBO_FORMAT_STRING);
        obs_property_list_add_string(edge_mode, "Black Padding", "padding");
        obs_property_list_add_string(edge_mode, "Crop Borders", "crop");
        obs_property_list_add_string(edge_mode, "Scale to Fit", "scale");

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

        // Edge handling default (Issue #226)
        obs_data_set_default_string(settings, "edge_handling", "padding");

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
    
    // NOTE: const_cast is required because OBS API functions expect non-const obs_data_t* parameters.
    // This is safe because we are only reading values from the settings object, not modifying it.
    // The OBS API is designed to work with const pointers that can be cast to non-const for reading.
    // This is a known pattern in OBS plugin development.
    
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

    // Edge handling (Issue #226)
    const char* edge_str = obs_data_get_string(const_cast<obs_data_t*>(settings), "edge_handling");
    if (strcmp(edge_str, "crop") == 0) {
        params.edge_mode = StabilizerCore::EdgeMode::Crop;
    } else if (strcmp(edge_str, "scale") == 0) {
        params.edge_mode = StabilizerCore::EdgeMode::Scale;
    } else {
        params.edge_mode = StabilizerCore::EdgeMode::Padding; // Default
    }

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

    // Edge handling (Issue #226)
    const char* edge_str = "padding"; // Default
    switch (params.edge_mode) {
        case StabilizerCore::EdgeMode::Crop:
            edge_str = "crop";
            break;
        case StabilizerCore::EdgeMode::Scale:
            edge_str = "scale";
            break;
        case StabilizerCore::EdgeMode::Padding:
        default:
            edge_str = "padding";
            break;
    }
    obs_data_set_string(settings, "edge_handling", edge_str);
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

/**
 * Converts an OpenCV Mat to an OBS source frame using centralized utilities.
 *
 * This function creates a complete copy of frame data and does NOT modify the reference frame.
 * Uses FRAME_UTILS::FrameBuffer for thread-safe buffer management.
 *
 * @param mat The OpenCV matrix to convert
 * @param reference_frame The reference frame to copy metadata from
 * @return Pointer to internal buffer with the converted data, or nullptr on error
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
#endif // HAVE_OBS_HEADERS

#ifdef HAVE_OBS_HEADERS
// Plugin entry points - C linkage required for OBS to find these functions
extern "C" {

MODULE_EXPORT const char *obs_module_name(void)
{
    return "Video Stabilizer";
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Real-time video stabilization plugin for OBS Studio using OpenCV";
}

MODULE_EXPORT bool obs_module_load(void)
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

MODULE_EXPORT void obs_module_unload(void)
{
    obs_log(LOG_INFO, "OBS Stabilizer Plugin unloaded");
}

}

MODULE_EXPORT void obs_module_set_pointer(obs_module_t *module)
{
    (void)module;
}
#endif // HAVE_OBS_HEADERS


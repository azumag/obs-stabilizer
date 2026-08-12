/*
 * OBS Stabilizer Plugin - Refactored with Modular Architecture
 * Uses the new modular design with StabilizerCore and OBSIntegration layers
 * Maintains compatibility with existing OBS API structure
 */

#include <opencv2/opencv.hpp>

#ifdef HAVE_OBS_HEADERS
#include "obs_compat.h"
#include "core/frame_utils.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("azumag")
#endif

#include "core/stabilizer_core.hpp"
#include "core/stabilizer_wrapper.hpp"
#include "core/parameter_validation.hpp"
#include "core/resolution_profile.hpp"
#include "core/preset_manager.hpp"
#include "core/rolling_average.hpp"
#include "ui/stabilizer_filter_context.hpp"
#include "ui/stabilizer_properties.hpp"
#include <memory>
#include <cstring>
#include <chrono>

// OBS module declarations - using existing macros from stub headers

#ifdef HAVE_OBS_HEADERS
// Forward declarations
static const char *stabilizer_filter_name(void *unused);
static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source);
static void stabilizer_filter_destroy(void *data);
static void stabilizer_filter_update(void *data, obs_data_t *settings);
static obs_source_frame *stabilizer_filter_video(void *data, obs_source_frame *frame);
static void apply_resolution_profile(const obs_source_frame *frame,
                                     StabilizerCore::StabilizerParams *params);
static obs_properties_t *stabilizer_filter_properties(void *data);

// Frame conversion functions
static cv::Mat obs_frame_to_cv_mat(const obs_source_frame *frame);

// Plugin structure definition
static struct obs_source_info stabilizer_filter_info = {
    .id = "stabilizer_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    // filter_video receives raw frames only when the filter is marked async.
    // Without OBS_SOURCE_ASYNC OBS lists this as an effect filter but never
    // invokes the OpenCV processing callback for async media sources.
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC,
    .get_name = stabilizer_filter_name,
    .create = stabilizer_filter_create,
    .destroy = stabilizer_filter_destroy,
    .get_defaults = set_stabilizer_defaults,
    .get_properties = stabilizer_filter_properties,
    .update = stabilizer_filter_update,
    .video_render = NULL,
    .filter_video = stabilizer_filter_video,
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
        context->processing_time_average.reset();

        // Get initial parameters
        // Note: Parameters are already validated via VALIDATION::validate_parameters in settings_to_params()
        context->params = settings_to_params(settings);

        blog(LOG_INFO, "[obs-stabilizer] Stabilizer filter created successfully");
        return context.release();

    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[obs-stabilizer] Exception in filter create: %s", e.what());
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
        blog(LOG_INFO, "[obs-stabilizer] Stabilizer filter destroyed");

    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[obs-stabilizer] Exception in filter destroy: %s", e.what());
    }
}

static void stabilizer_filter_update(void *data, obs_data_t *settings)
{
    try {
        struct stabilizer_filter *context = (struct stabilizer_filter *)data;
        if (!context) {
            blog(LOG_ERROR, "[obs-stabilizer] Invalid context in filter update");
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
        blog(LOG_ERROR, "[obs-stabilizer] Exception in filter update: %s", e.what());
    }
}

static obs_source_frame *stabilizer_filter_video(void *data, obs_source_frame *frame)
{
    try {
        struct stabilizer_filter *context = (struct stabilizer_filter *)data;
        if (!context || !frame) {
            return frame;
        }

        // Initialize stabilizer on first frame
        if (!context->initialized) {
            // Apply a resolution-aware parameter profile on first frame so the
            // feature count and smoothing window stay sane at 4K and above.
            apply_resolution_profile(frame, &context->params);
            if (!context->stabilizer.initialize(frame->width, frame->height, context->params)) {
                blog(LOG_ERROR, "[obs-stabilizer] Failed to initialize stabilizer: %s",
                         context->stabilizer.get_last_error().c_str());
                return frame;
            }

            context->initialized = true;
            blog(LOG_INFO, "[obs-stabilizer] Stabilizer initialized for %dx%d", frame->width, frame->height);
        }

        // Convert OBS frame to OpenCV Mat
        cv::Mat cv_frame = obs_frame_to_cv_mat(frame);
        if (cv_frame.empty()) {
            blog(LOG_ERROR, "[obs-stabilizer] Failed to convert OBS frame to OpenCV Mat");
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
        context->processing_time_average.add(processing_time);
        context->avg_processing_time = context->processing_time_average.value();

        // OBS owns and reference-counts the frame passed to an async filter.
        // Replacing that pointer with a plugin allocation breaks OBS's async
        // frame queue, so copy stabilized pixels back into the same frame.
        if (!FRAME_UTILS::Conversion::cv_to_obs_in_place(stabilized_frame, frame)) {
            blog(LOG_ERROR, "[obs-stabilizer] Failed to copy stabilized pixels to OBS frame");
        }
        return frame;

    } catch (const std::exception& e) {
        blog(LOG_ERROR, "[obs-stabilizer] Exception in video processing: %s", e.what());
        return frame;
    }
}


static void apply_resolution_profile(const obs_source_frame *frame,
                                     StabilizerCore::StabilizerParams *params)
{
    if (!frame || !params) {
        return;
    }

    const stabilizer::ResolutionProfile profile =
        stabilizer::make_resolution_profile(frame->width, frame->height);
    if (!profile.valid) {
        return;
    }

    params->feature_count = static_cast<int>(profile.feature_count);
    // Keep the user's smoothing setting; the profile only auto-tunes the
    // feature budget so higher resolutions do not stall the pipeline.
    blog(LOG_INFO, "[obs-stabilizer] Applied resolution profile for %ux%u "
                   "(features: %u)",
         frame->width, frame->height, profile.feature_count);
}

static obs_properties_t *stabilizer_filter_properties(void *data)
{
    return build_stabilizer_properties(static_cast<struct stabilizer_filter *>(data));
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
        blog(LOG_ERROR, "[obs-stabilizer] OpenCV exception in obs_frame_to_cv_mat: %s", e.what());
        return cv::Mat();
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
    blog(LOG_INFO, "[obs-stabilizer] Loading OBS Stabilizer Plugin (Modular Architecture)");
    obs_register_source(&stabilizer_filter_info);
    blog(LOG_INFO, "[obs-stabilizer] OBS Stabilizer Plugin loaded successfully");
    return true;
}

MODULE_EXPORT void obs_module_unload(void)
{
    blog(LOG_INFO, "[obs-stabilizer] OBS Stabilizer Plugin unloaded");
}

}
#endif // HAVE_OBS_HEADERS

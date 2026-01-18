/*
OBS Stabilizer Plugin - Minimal Test Implementation (No OpenCV)
Architectural simplification following Gemini review requirements
*/

#include <obs-module.h>
#include <stdio.h>

struct StabilizerFilter {
    bool enabled;
    int smoothing_radius;
};

static const char* stabilizer_filter_name(void* unused) {
    return "Stabilizer";
}

static void* stabilizer_filter_create(obs_data_t* settings, obs_source_t* source) {
    StabilizerFilter* filter = new StabilizerFilter();
    
    // Load initial settings
    filter->enabled = obs_data_get_bool(settings, "enable_stabilization");
    filter->smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
    
    obs_log(LOG_INFO, "[obs-stabilizer] Stabilizer filter created (minimal test version)");
    return filter;
}

static void stabilizer_filter_destroy(void* data) {
    if (!data) return;
    
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    delete filter;
    
    obs_log(LOG_INFO, "[obs-stabilizer] Stabilizer filter destroyed (minimal test version)");
}

static void stabilizer_filter_update(void* data, obs_data_t* settings) {
    if (!data) return;
    
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    
    // Update configuration
    filter->enabled = obs_data_get_bool(settings, "enable_stabilization");
    filter->smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
}

static struct obs_source_frame* stabilizer_filter_video(void* data, struct obs_source_frame* frame) {
    if (!data || !frame) return frame;
    
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    
    // Minimal test - just pass through the frame
    // In a real implementation, this would apply stabilization
    if (filter->enabled) {
        // Placeholder for stabilization logic
    }
    
    return frame;
}

static obs_properties_t* stabilizer_filter_properties(void* data) {
    obs_properties_t* props = obs_properties_create();
    
    obs_properties_add_bool(props, "enable_stabilization", "Enable Stabilization");
    obs_properties_add_int_slider(props, "smoothing_radius", "Smoothing Radius", 10, 100, 5);
    
    return props;
}

static void stabilizer_filter_defaults(obs_data_t* settings) {
    obs_data_set_default_bool(settings, "enable_stabilization", true);
    obs_data_set_default_int(settings, "smoothing_radius", 30);
}

// Filter definition
static struct obs_source_info stabilizer_filter_info = {
    "stabilizer_filter",
    OBS_SOURCE_TYPE_FILTER,
    OBS_SOURCE_VIDEO,
    stabilizer_filter_name,
    stabilizer_filter_create,
    stabilizer_filter_destroy,
    NULL,
    NULL,
    stabilizer_filter_update,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    stabilizer_filter_video,
    stabilizer_filter_properties,
    stabilizer_filter_defaults,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

void register_stabilizer_filter() {
    obs_register_source(&stabilizer_filter_info);
}
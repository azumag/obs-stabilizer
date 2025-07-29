/*
OBS Stabilizer Plugin - Simplified OBS Integration
Architectural simplification following Gemini review requirements
*/

#include <obs-module.h>
#include "stabilizer.cpp" // Include the core logic

struct StabilizerFilter {
    VideoStabilizer* stabilizer;
    StabilizerConfig config;
};

static const char* stabilizer_filter_name(void* unused) {
    return "Stabilizer";
}

static void* stabilizer_filter_create(obs_data_t* settings, obs_source_t* source) {
    StabilizerFilter* filter = new StabilizerFilter();
    filter->stabilizer = new VideoStabilizer();

    // Load initial settings
    filter->config.enable_stabilization = obs_data_get_bool(settings, "enable_stabilization");
    filter->config.smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
    filter->config.max_features = (int)obs_data_get_int(settings, "max_features");
    filter->config.feature_quality = obs_data_get_double(settings, "feature_quality");
    filter->config.min_distance = obs_data_get_double(settings, "min_distance");
    filter->config.detection_interval = (int)obs_data_get_int(settings, "detection_interval");

    filter->stabilizer->update_config(filter->config);
    
    obs_log(LOG_INFO, "Stabilizer filter created");
    return filter;
}

static void stabilizer_filter_destroy(void* data) {
    if (!data) return;
    
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    delete filter->stabilizer;
    delete filter;
    
    obs_log(LOG_INFO, "Stabilizer filter destroyed");
}

static void stabilizer_filter_update(void* data, obs_data_t* settings) {
    if (!data) return;
    
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    
    // Update configuration
    filter->config.enable_stabilization = obs_data_get_bool(settings, "enable_stabilization");
    filter->config.smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
    filter->config.max_features = (int)obs_data_get_int(settings, "max_features");
    filter->config.feature_quality = obs_data_get_double(settings, "feature_quality");
    filter->config.min_distance = obs_data_get_double(settings, "min_distance");
    filter->config.detection_interval = (int)obs_data_get_int(settings, "detection_interval");
    
    filter->stabilizer->update_config(filter->config);
}

static struct obs_source_frame* stabilizer_filter_video(void* data, struct obs_source_frame* frame) {
    if (!data || !frame) return frame;
    
    StabilizerFilter* filter = static_cast<StabilizerFilter*>(data);
    
    // Process frame for stabilization
    filter->stabilizer->process_frame(frame);
    
    return frame;
}

static obs_properties_t* stabilizer_filter_properties(void* data) {
    obs_properties_t* props = obs_properties_create();
    
    obs_properties_add_bool(props, "enable_stabilization", "Enable Stabilization");
    
    obs_properties_add_int_slider(props, "smoothing_radius", "Smoothing Radius", 
                                  10, 100, 5);
    
    obs_properties_add_int_slider(props, "max_features", "Max Features", 
                                  50, 500, 10);
    
    obs_properties_add_float_slider(props, "feature_quality", "Feature Quality", 
                                    0.001, 0.1, 0.001);
    
    obs_properties_add_float_slider(props, "min_distance", "Min Distance", 
                                    10.0, 100.0, 5.0);
    
    obs_properties_add_int_slider(props, "detection_interval", "Detection Interval", 
                                  5, 30, 1);
    
    return props;
}

static void stabilizer_filter_defaults(obs_data_t* settings) {
    obs_data_set_default_bool(settings, "enable_stabilization", true);
    obs_data_set_default_int(settings, "smoothing_radius", 30);
    obs_data_set_default_int(settings, "max_features", 200);
    obs_data_set_default_double(settings, "feature_quality", 0.01);
    obs_data_set_default_double(settings, "min_distance", 30.0);
    obs_data_set_default_int(settings, "detection_interval", 10);
}

// Filter definition
static struct obs_source_info stabilizer_filter_info = {
    .id = "stabilizer_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = stabilizer_filter_name,
    .create = stabilizer_filter_create,
    .destroy = stabilizer_filter_destroy,
    .update = stabilizer_filter_update,
    .filter_video = stabilizer_filter_video,
    .get_properties = stabilizer_filter_properties,
    .get_defaults = stabilizer_filter_defaults,
};

void register_stabilizer_filter() {
    obs_register_source(&stabilizer_filter_info);
}
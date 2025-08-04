/*
OBS Stabilizer Plugin - Video Filter Implementation
Real-time video stabilization using OpenCV
*/

#include <obs-module.h>
#include <pthread.h>
#include <stdlib.h>
#include "stabilizer.h"

// Filter data structure
struct stabilizer_filter_data {
    obs_source_t *source;
    VideoStabilizer *stabilizer;
    StabilizerConfig config;
    pthread_mutex_t mutex;
};

// Filter name and ID
static const char *stabilizer_filter_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "Video Stabilizer";
}

// Create filter instance
static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source)
{
    struct stabilizer_filter_data *data = (struct stabilizer_filter_data *)calloc(1, sizeof(struct stabilizer_filter_data));
    
    if (!data) return NULL;
    
    data->source = source;
    data->stabilizer = new VideoStabilizer();
    
    // Initialize configuration with default values
    data->config.enable_stabilization = obs_data_get_bool(settings, "enable_stabilization");
    data->config.smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
    data->config.max_features = (int)obs_data_get_int(settings, "max_features");
    data->config.feature_quality = obs_data_get_double(settings, "feature_quality");
    data->config.min_distance = obs_data_get_double(settings, "min_distance");
    data->config.detection_interval = (int)obs_data_get_int(settings, "detection_interval");
    
    data->stabilizer->update_config(data->config);
    
    pthread_mutex_init(&data->mutex, NULL);
    
    return data;
}

// Destroy filter instance
static void stabilizer_filter_destroy(void *data)
{
    struct stabilizer_filter_data *filter = (struct stabilizer_filter_data *)data;
    
    if (filter) {
        pthread_mutex_destroy(&filter->mutex);
        delete filter->stabilizer;
        free(filter);
    }
}

// Update filter settings
static void stabilizer_filter_update(void *data, obs_data_t *settings)
{
    struct stabilizer_filter_data *filter = (struct stabilizer_filter_data *)data;
    
    pthread_mutex_lock(&filter->mutex);
    
    filter->config.enable_stabilization = obs_data_get_bool(settings, "enable_stabilization");
    filter->config.smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
    filter->config.max_features = (int)obs_data_get_int(settings, "max_features");
    filter->config.feature_quality = obs_data_get_double(settings, "feature_quality");
    filter->config.min_distance = obs_data_get_double(settings, "min_distance");
    filter->config.detection_interval = (int)obs_data_get_int(settings, "detection_interval");
    
    filter->stabilizer->update_config(filter->config);
    
    pthread_mutex_unlock(&filter->mutex);
}

// Define filter properties (UI)
static obs_properties_t *stabilizer_filter_properties(void *data)
{
    UNUSED_PARAMETER(data);
    
    obs_properties_t *props = obs_properties_create();
    
    obs_properties_add_bool(props, "enable_stabilization", "Enable Stabilization");
    obs_properties_add_int_slider(props, "smoothing_radius", "Smoothing Radius", 1, 100, 1);
    obs_properties_add_int_slider(props, "max_features", "Max Features", 50, 500, 10);
    obs_properties_add_float_slider(props, "feature_quality", "Feature Quality", 0.001, 0.1, 0.001);
    obs_properties_add_float_slider(props, "min_distance", "Min Distance", 10.0, 100.0, 1.0);
    obs_properties_add_int_slider(props, "detection_interval", "Detection Interval", 5, 50, 1);
    
    return props;
}

// Set default values
static void stabilizer_filter_defaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, "enable_stabilization", true);
    obs_data_set_default_int(settings, "smoothing_radius", 30);
    obs_data_set_default_int(settings, "max_features", 200);
    obs_data_set_default_double(settings, "feature_quality", 0.01);
    obs_data_set_default_double(settings, "min_distance", 30.0);
    obs_data_set_default_int(settings, "detection_interval", 10);
}

// Main filter processing function
static struct obs_source_frame *stabilizer_filter_video(void *data, struct obs_source_frame *frame)
{
    struct stabilizer_filter_data *filter = (struct stabilizer_filter_data *)data;
    
    if (!filter || !frame) {
        return frame;
    }
    
    pthread_mutex_lock(&filter->mutex);
    
    // Process frame with stabilizer
    if (filter->config.enable_stabilization) {
        bool success = filter->stabilizer->process_frame(frame);
        if (!success) {
            obs_log(LOG_DEBUG, "Stabilization processing failed, passing frame through");
        }
    }
    
    pthread_mutex_unlock(&filter->mutex);
    
    return frame;
}

// Filter info structure
static struct obs_source_info stabilizer_filter_info = {
    .id = "video_stabilizer_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = stabilizer_filter_get_name,
    .create = stabilizer_filter_create,
    .destroy = stabilizer_filter_destroy,
    .update = stabilizer_filter_update,
    .get_properties = stabilizer_filter_properties,
    .get_defaults = stabilizer_filter_defaults,
    .filter_video = stabilizer_filter_video,
};

// Register the filter
extern "C" {
    void register_stabilizer_filter(void)
    {
        obs_register_source(&stabilizer_filter_info);
    }
}
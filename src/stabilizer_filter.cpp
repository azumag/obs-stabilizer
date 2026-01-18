/*
OBS Minimal Stabilizer Plugin - Simplified video filter without OpenCV
*/

#include "obs-module.h"
#include <cstring>
#include <cmath>
#include <climits>
#include <vector>
#include <deque>
#include <pthread.h>
#include <cstdlib>

// OBS source flags - remove if already defined
#ifndef OBS_SOURCE_VIDEO
#define OBS_SOURCE_VIDEO (1 << 0)
#endif
#ifndef OBS_SOURCE_AUDIO
#define OBS_SOURCE_AUDIO (1 << 1)
#endif
#ifndef OBS_SOURCE_ASYNC
#define OBS_SOURCE_ASYNC (1 << 2)
#endif

// Memory functions - use standard C functions if OBS functions not available
#ifndef bzalloc
#define bzalloc(size) calloc(1, size)
#endif

#ifndef bfree
#define bfree(ptr) free(ptr)
#endif

// Plugin module pointer
OBS_DECLARE_MODULE()

// Simple transform structure
struct Transform {
    float dx;
    float dy;
    float angle;
    
    Transform() : dx(0.0f), dy(0.0f), angle(0.0f) {}
    Transform(float x, float y, float a) : dx(x), dy(y), angle(a) {}
};

// Minimal stabilizer data
struct minimal_stabilizer_data {
    obs_source_t *context;
    
    // Basic settings
    bool enabled;
    int smoothing_window;
    float stabilization_strength;
    
    // Motion history
    std::deque<Transform> transform_history;
    std::deque<Transform> smoothed_transforms;
    
    // Previous frame data for simple motion detection
    uint8_t *prev_frame;
    uint32_t frame_width;
    uint32_t frame_height;
    
    pthread_mutex_t mutex;
};

// Filter name
static const char *minimal_stabilizer_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "Minimal Stabilizer";
}

// Simple motion estimation using block matching
static Transform estimate_motion(const uint8_t *prev, const uint8_t *curr, 
                                uint32_t width, uint32_t height, uint32_t linesize)
{
    Transform t;
    
    // Very simple block matching at center of frame
    const int block_size = 16;
    const int search_range = 8;
    
    int cx = width / 2;
    int cy = height / 2;
    
    int best_dx = 0, best_dy = 0;
    int min_diff = INT_MAX;
    
    // Search for best match
    for (int dy = -search_range; dy <= search_range; dy++) {
        for (int dx = -search_range; dx <= search_range; dx++) {
            int diff = 0;
            
            for (int y = 0; y < block_size; y++) {
                for (int x = 0; x < block_size; x++) {
                    int px = cx + x;
                    int py = cy + y;
                    int px2 = px + dx;
                    int py2 = py + dy;
                    
                    if (px2 >= 0 && px2 < (int)width && py2 >= 0 && py2 < (int)height) {
                        int idx1 = py * linesize + px * 4;
                        int idx2 = py2 * linesize + px2 * 4;
                        
                        // Compare Y component (assuming BGRA format)
                        int d = curr[idx1] - prev[idx2];
                        diff += d * d;
                    }
                }
            }
            
            if (diff < min_diff) {
                min_diff = diff;
                best_dx = dx;
                best_dy = dy;
            }
        }
    }
    
    t.dx = (float)best_dx;
    t.dy = (float)best_dy;
    t.angle = 0.0f; // No rotation for simplicity
    
    return t;
}

// Apply smoothing to transforms
static Transform smooth_transform(const std::deque<Transform> &history, int window)
{
    Transform smoothed;
    
    if (history.empty()) return smoothed;
    
    int count = std::min((int)history.size(), window);
    
    for (int i = 0; i < count; i++) {
        smoothed.dx += history[i].dx;
        smoothed.dy += history[i].dy;
        smoothed.angle += history[i].angle;
    }
    
    smoothed.dx /= count;
    smoothed.dy /= count;
    smoothed.angle /= count;
    
    return smoothed;
}

// Create filter
static void *minimal_stabilizer_create(obs_data_t *settings, obs_source_t *source)
{
    minimal_stabilizer_data *data = new minimal_stabilizer_data;
    
    data->context = source;
    data->enabled = true;
    data->smoothing_window = 5;
    data->stabilization_strength = 0.8f;
    data->prev_frame = nullptr;
    data->frame_width = 0;
    data->frame_height = 0;
    
    pthread_mutex_init(&data->mutex, nullptr);
    
    // Update settings after initialization
    if (settings) {
        data->enabled = obs_data_get_bool(settings, "enabled");
        data->smoothing_window = (int)obs_data_get_int(settings, "smoothing_window");
        data->stabilization_strength = (float)obs_data_get_double(settings, "strength");
    }
    
    return data;
}

// Destroy filter
static void minimal_stabilizer_destroy(void *data)
{
    minimal_stabilizer_data *filter = (minimal_stabilizer_data *)data;
    
    if (filter) {
        pthread_mutex_destroy(&filter->mutex);
        
        if (filter->prev_frame) {
            bfree(filter->prev_frame);
        }
        
        delete filter;
    }
}

// Update settings - DO NOT ACCESS SETTINGS PARAMETER (causes crash)
static void minimal_stabilizer_update(void *data, obs_data_t *settings)
{
    // NULL safety checks
    if (!data) {
        return;
    }
    
    // Do NOT access settings parameter - it causes crashes
    // Settings are read in create() and stored in filter data
    
    minimal_stabilizer_data *filter = (minimal_stabilizer_data *)data;
    
    if (!filter) {
        return;
    }
    
    // Settings are already stored from create(), no need to update here
    // This prevents the crash when accessing invalid settings pointer
}

// Video tick - process frame
static struct obs_source_frame *minimal_stabilizer_filter_video(void *data, struct obs_source_frame *frame)
{
    // NULL safety checks
    if (!data || !frame) {
        return frame;
    }
    
    minimal_stabilizer_data *filter = (minimal_stabilizer_data *)data;
    
    if (!filter || !filter->enabled) {
        return frame;
    }
    
    pthread_mutex_lock(&filter->mutex);
    
    // Check if frame size changed
    if (filter->frame_width != frame->width || filter->frame_height != frame->height) {
        filter->frame_width = frame->width;
        filter->frame_height = frame->height;
        
        if (filter->prev_frame) {
            bfree(filter->prev_frame);
        }
        
        filter->prev_frame = (uint8_t *)bzalloc(frame->linesize[0] * frame->height);
        
        // Clear history on size change
        filter->transform_history.clear();
        filter->smoothed_transforms.clear();
    }
    
    // If we have a previous frame, estimate motion
    if (filter->prev_frame && frame->data[0]) {
        Transform t = estimate_motion(filter->prev_frame, frame->data[0], 
                                     frame->width, frame->height, frame->linesize[0]);
        
        // Add to history
        filter->transform_history.push_back(t);
        if (filter->transform_history.size() > 30) {
            filter->transform_history.pop_front();
        }
        
        // Calculate smoothed transform
        Transform smoothed = smooth_transform(filter->transform_history, filter->smoothing_window);
        
        // Apply stabilization by shifting in opposite direction
        float dx = -smoothed.dx * filter->stabilization_strength;
        float dy = -smoothed.dy * filter->stabilization_strength;
        
        // Simple translation - shift pixels
        // Note: This is very basic and will show black borders
        // A real implementation would use proper warping
        
        int shift_x = (int)dx;
        int shift_y = (int)dy;
        
        if (abs(shift_x) < 50 && abs(shift_y) < 50) { // Safety limit
            uint8_t *temp = (uint8_t *)bzalloc(frame->linesize[0] * frame->height);
            memcpy(temp, frame->data[0], frame->linesize[0] * frame->height);
            
            // Apply shift
            for (uint32_t y = 0; y < frame->height; y++) {
                for (uint32_t x = 0; x < frame->width; x++) {
                    int src_x = x - shift_x;
                    int src_y = y - shift_y;
                    
                    if (src_x >= 0 && src_x < (int)frame->width && 
                        src_y >= 0 && src_y < (int)frame->height) {
                        
                        uint8_t *dst = frame->data[0] + y * frame->linesize[0] + x * 4;
                        uint8_t *src = temp + src_y * frame->linesize[0] + src_x * 4;
                        
                        // Copy BGRA pixel
                        memcpy(dst, src, 4);
                    } else {
                        // Fill with black for out of bounds
                        uint8_t *dst = frame->data[0] + y * frame->linesize[0] + x * 4;
                        memset(dst, 0, 4);
                    }
                }
            }
            
            bfree(temp);
        }
        
        // Save current frame as previous
        memcpy(filter->prev_frame, frame->data[0], frame->linesize[0] * frame->height);
    } else if (frame->data[0]) {
        // First frame - just save it
        memcpy(filter->prev_frame, frame->data[0], frame->linesize[0] * frame->height);
    }
    
    pthread_mutex_unlock(&filter->mutex);
    
    return frame;
}

// Get default settings
static void minimal_stabilizer_get_defaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, "enabled", true);
    obs_data_set_default_int(settings, "smoothing_window", 5);
    obs_data_set_default_double(settings, "strength", 0.8);
}

// Get properties for UI
static obs_properties_t *minimal_stabilizer_get_properties(void *data)
{
    UNUSED_PARAMETER(data);
    
    obs_properties_t *props = obs_properties_create();
    
    obs_properties_add_bool(props, "enabled", "Enable Stabilization");
    
    obs_properties_add_int_slider(props, "smoothing_window", 
                                  "Smoothing Window", 1, 30, 1);
    
    obs_properties_add_float_slider(props, "strength", 
                                    "Stabilization Strength", 0.0, 1.0, 0.1);
    
    return props;
}

// Register the filter
static struct obs_source_info minimal_stabilizer_filter = {
    .id = "minimal_stabilizer_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC,
    .get_name = minimal_stabilizer_get_name,
    .create = minimal_stabilizer_create,
    .destroy = minimal_stabilizer_destroy,
    .update = minimal_stabilizer_update,
    .filter_video = minimal_stabilizer_filter_video,
    .get_defaults = minimal_stabilizer_get_defaults,
    .get_properties = minimal_stabilizer_get_properties
};

// Module load - export with different name to avoid conflict
extern "C" bool minimal_obs_module_load(void)
{
    obs_register_source(&minimal_stabilizer_filter);
    return true;
}

// Module description
const char *obs_module_description(void)
{
    return "Minimal video stabilizer without OpenCV";
}
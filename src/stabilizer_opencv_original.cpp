/*
OBS Stabilizer with OpenCV - Production Implementation
Uses workaround for settings crash: only read settings in create, not update
*/

#include <obs-module.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/features2d.hpp>
#include <mutex>
#include <memory>
#include <deque>
#include <vector>
#include <chrono>
#include "stabilizer_constants.h"

// Forward declarations
static void apply_preset(obs_data_t *settings, const char *preset_name);

struct stabilizer_filter {
    obs_source_t *source;
    
    // Parameters - stored from create function to avoid settings crash
    bool enabled;
    int smoothing_radius;         // Number of frames to average for smoothing (10-100 recommended)
    float max_correction;         // Maximum correction percentage (10.0-100.0)
    int feature_count;            // Number of feature points to track (100-1000 recommended)
    float quality_level;          // Minimal accepted quality of corners (0.001-0.1)
    float min_distance;           // Minimum possible Euclidean distance between corners (10-100 pixels)
    int block_size;               // Size of an average block for computing a derivative covariation matrix (3 recommended)
    bool use_harris;              // Use Harris detector instead of cornerMinEigenVal
    float k;                      // Free parameter of Harris detector (0.04 recommended)
    int winSize;                  // Size of the search window at each pyramid level (15-31 recommended)
    int maxLevel;                 // 0-based maximal pyramid level number (3 recommended)
    int maxCount;                 // Maximum number of iterations (20-30 recommended)
    float epsilon;                // Desired accuracy (0.01 recommended)
    float minEigThreshold;       // Minimal eigen threshold (0.0001-0.001)
    
    // OpenCV data for stabilization
    cv::Mat prev_gray;
    std::vector<cv::Point2f> prev_pts;
    std::deque<cv::Mat> transforms;
    cv::Mat cumulative_transform;
    bool first_frame;
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point last_frame_time;
    double avg_processing_time;
    int frame_count;
    
    // Thread safety
    std::mutex mutex;
    
    // Debug mode
    bool debug_mode;
    
    // Frame dimensions
    uint32_t width;
    uint32_t height;
};

static const char *stabilizer_filter_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "OpenCV Stabilizer";
}

static cv::Mat calculate_transform(const std::vector<cv::Point2f> &prev_pts,
                                  const std::vector<cv::Point2f> &curr_pts)
{
    if (prev_pts.size() < 4 || curr_pts.size() < 4) {
        return cv::Mat::eye(3, 3, CV_64F);
    }
    
    // Estimate rigid transform
    cv::Mat transform = cv::estimateAffinePartial2D(prev_pts, curr_pts);
    
    if (transform.empty()) {
        return cv::Mat::eye(3, 3, CV_64F);
    }
    
    // Convert to 3x3 homography matrix
    cv::Mat H = cv::Mat::eye(3, 3, CV_64F);
    transform.copyTo(H(cv::Rect(0, 0, 3, 2)));
    
    return H;
}

static cv::Mat smooth_transform(const std::deque<cv::Mat> &transforms, int radius)
{
    if (transforms.empty()) {
        return cv::Mat::eye(3, 3, CV_64F);
    }
    
    cv::Mat avg = cv::Mat::zeros(3, 3, CV_64F);
    int count = 0;
    
    for (const auto &t : transforms) {
        if (!t.empty()) {
            avg += t;
            count++;
        }
    }
    
    if (count > 0) {
        avg /= count;
    } else {
        avg = cv::Mat::eye(3, 3, CV_64F);
    }
    
    return avg;
}

static void *stabilizer_filter_create(obs_data_t *settings, obs_source_t *source)
{
    obs_log(LOG_INFO, "Creating OpenCV stabilizer filter");
    
    struct stabilizer_filter *filter = (struct stabilizer_filter *)calloc(1, sizeof(struct stabilizer_filter));
    filter->source = source;
    
    // WORKAROUND: Read all settings in create to avoid crash in update
    // Default values
    filter->enabled = true;
    filter->smoothing_radius = 30;
    filter->max_correction = 50.0f;
    filter->feature_count = 200;
    filter->quality_level = 0.01f;
    filter->min_distance = 30.0f;
    filter->block_size = 3;
    filter->use_harris = false;
    filter->k = 0.04f;
    filter->winSize = 30;
    filter->maxLevel = 3;
    filter->maxCount = 30;
    filter->epsilon = 0.01f;
    filter->minEigThreshold = 0.0001f;
    filter->debug_mode = false;
    
    // Read settings if available (safe in create function)
    if (settings) {
        // Handle preset first
        const char *preset = obs_data_get_string(settings, "preset");
        if (preset && strlen(preset) > 0 && strcmp(preset, "custom") != 0) {
            apply_preset(settings, preset);
            obs_log(LOG_INFO, "Applied preset: %s", preset);
        }
        
        filter->enabled = obs_data_get_bool(settings, "enabled");
        filter->smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
        filter->max_correction = (float)obs_data_get_double(settings, "max_correction");
        filter->feature_count = (int)obs_data_get_int(settings, "feature_count");
        filter->quality_level = (float)obs_data_get_double(settings, "quality_level");
        filter->min_distance = (float)obs_data_get_double(settings, "min_distance");
        filter->debug_mode = obs_data_get_bool(settings, "debug_mode");
        
        obs_log(LOG_INFO, "Loaded settings - enabled: %s, smoothing: %d, features: %d, preset: %s",
                filter->enabled ? "true" : "false",
                filter->smoothing_radius,
                filter->feature_count,
                preset ? preset : "none");
    }
    
    // Initialize OpenCV structures
    filter->cumulative_transform = cv::Mat::eye(3, 3, CV_64F);
    filter->avg_processing_time = 0.0;
    filter->frame_count = 0;
    filter->first_frame = true;
    filter->width = 0;
    filter->height = 0;
    
    obs_log(LOG_INFO, "OpenCV stabilizer filter created successfully");
    return filter;
}

static void stabilizer_filter_destroy(void *data)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    
    if (filter) {
        obs_log(LOG_INFO, "Destroying OpenCV stabilizer filter");
        free(filter);
    }
}

static void stabilizer_filter_update(void *data, obs_data_t *settings)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (!filter) {
        obs_log(LOG_WARNING, "Update called with NULL filter data");
        return;
    }
    
    // Validate settings pointer before access
    if (!settings) {
        obs_log(LOG_WARNING, "Update called with NULL settings");
        return;
    }
    
    std::lock_guard<std::mutex> lock(filter->mutex);
    
    // Handle preset changes
    const char *preset = obs_data_get_string(settings, "preset");
    if (preset && strlen(preset) > 0 && strcmp(preset, "custom") != 0) {
        apply_preset(settings, preset);
        obs_log(LOG_INFO, "Applied preset: %s", preset);
    }
    
    // Update parameters safely
    filter->enabled = obs_data_get_bool(settings, "enabled");
    filter->smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
    filter->max_correction = (float)obs_data_get_double(settings, "max_correction");
    filter->feature_count = (int)obs_data_get_int(settings, "feature_count");
    filter->quality_level = (float)obs_data_get_double(settings, "quality_level");
    filter->min_distance = (float)obs_data_get_double(settings, "min_distance");
    filter->debug_mode = obs_data_get_bool(settings, "debug_mode");
    
    // Validate parameter ranges
    if (filter->smoothing_radius < SAFETY::MIN_SMOOTHING_OVERRIDE) filter->smoothing_radius = SAFETY::MIN_SMOOTHING_OVERRIDE;
    if (filter->smoothing_radius > SAFETY::MAX_SMOOTHING_OVERRIDE) filter->smoothing_radius = SAFETY::MAX_SMOOTHING_OVERRIDE;
    if (filter->max_correction < SAFETY::MIN_CORRECTION_OVERRIDE) filter->max_correction = SAFETY::MIN_CORRECTION_OVERRIDE;
    if (filter->max_correction > SAFETY::MAX_CORRECTION_OVERRIDE) filter->max_correction = SAFETY::MAX_CORRECTION_OVERRIDE;
    if (filter->feature_count < SAFETY::MIN_FEATURES_OVERRIDE) filter->feature_count = SAFETY::MIN_FEATURES_OVERRIDE;
    if (filter->feature_count > SAFETY::MAX_FEATURES_OVERRIDE) filter->feature_count = SAFETY::MAX_FEATURES_OVERRIDE;
    if (filter->quality_level < SAFETY::MIN_QUALITY_OVERRIDE) filter->quality_level = SAFETY::MIN_QUALITY_OVERRIDE;
    if (filter->quality_level > SAFETY::MAX_QUALITY_OVERRIDE) filter->quality_level = SAFETY::MAX_QUALITY_OVERRIDE;
    if (filter->min_distance < SAFETY::MIN_DISTANCE_OVERRIDE) filter->min_distance = SAFETY::MIN_DISTANCE_OVERRIDE;
    if (filter->min_distance > SAFETY::MAX_DISTANCE_OVERRIDE) filter->min_distance = SAFETY::MAX_DISTANCE_OVERRIDE;
    
    obs_log(LOG_DEBUG, "Settings updated - enabled: %s, smoothing: %d, features: %d",
            filter->enabled ? "true" : "false",
            filter->smoothing_radius,
            filter->feature_count);
}

static struct obs_source_frame *stabilizer_filter_video(void *data, struct obs_source_frame *frame)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;

    if (!filter || !filter->enabled || !frame) {
        return frame;
    }

    // Copy parameters needed for processing to minimize lock time
    bool enabled_copy;
    uint32_t width_copy, height_copy;
    int smoothing_radius_copy;
    float max_correction_copy;
    int feature_count_copy;
    float quality_level_copy;
    float min_distance_copy;
    bool debug_mode_copy;
    int block_size_copy;
    bool use_harris_copy;
    float k_copy;
    
    {
        std::lock_guard<std::mutex> lock(filter->mutex);
        enabled_copy = filter->enabled;
        width_copy = filter->width;
        height_copy = filter->height;
        smoothing_radius_copy = filter->smoothing_radius;
        max_correction_copy = filter->max_correction;
        feature_count_copy = filter->feature_count;
        quality_level_copy = filter->quality_level;
        min_distance_copy = filter->min_distance;
        debug_mode_copy = filter->debug_mode;
        block_size_copy = filter->block_size;
        use_harris_copy = filter->use_harris;
        k_copy = filter->k;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Get frame dimensions
        uint32_t width = frame->width;
        uint32_t height = frame->height;
        
        // Check if dimensions changed
        if (width_copy != width || height_copy != height) {
            {
                std::lock_guard<std::mutex> lock(filter->mutex);
                filter->width = width;
                filter->height = height;
                filter->first_frame = true;
                filter->prev_gray.release();
                filter->prev_pts.clear();
                filter->transforms.clear();
                filter->cumulative_transform = cv::Mat::eye(3, 3, CV_64F);
            }
            obs_log(LOG_INFO, "Frame dimensions changed to %dx%d", width, height);
        }
        
        // Convert frame to OpenCV Mat based on format
        cv::Mat current_frame;
        cv::Mat gray;
        
        if (frame->format == VIDEO_FORMAT_BGRA) {
            // BGRA format - most common
            current_frame = cv::Mat(height, width, CV_8UC4, frame->data[0], frame->linesize[0]);
            cv::cvtColor(current_frame, gray, cv::COLOR_BGRA2GRAY);
        } else if (frame->format == VIDEO_FORMAT_NV12) {
            // NV12 format - Y plane is already grayscale
            gray = cv::Mat(height, width, CV_8UC1, frame->data[0], frame->linesize[0]);
        } else if (frame->format == VIDEO_FORMAT_I420) {
            // I420 format - Y plane is already grayscale
            gray = cv::Mat(height, width, CV_8UC1, frame->data[0], frame->linesize[0]);
        } else {
            // Unsupported format, pass through
            if (debug_mode_copy) {
                obs_log(LOG_WARNING, "Unsupported video format: %d", frame->format);
            }
            return frame;
        }
        
        if (width_copy != width || height_copy != height) {
            // First frame after dimension change
            {
                std::lock_guard<std::mutex> lock(filter->mutex);
                cv::goodFeaturesToTrack(gray, filter->prev_pts, feature_count_copy,
                                       quality_level_copy, min_distance_copy,
                                       cv::Mat(), block_size_copy, use_harris_copy, k_copy);
                filter->prev_gray = gray.clone();
                filter->first_frame = false;
            }
            
            if (debug_mode_copy) {
                size_t pts_size;
                {
                    std::lock_guard<std::mutex> lock(filter->mutex);
                    pts_size = filter->prev_pts.size();
                }
                obs_log(LOG_INFO, "First frame processed, detected %zu features", pts_size);
            }
        } else {
            // Track features
            std::vector<cv::Point2f> curr_pts;
            std::vector<uchar> status;
            std::vector<float> err;
            
            {
                std::lock_guard<std::mutex> lock(filter->mutex);
                if (!filter->prev_pts.empty()) {
                    cv::calcOpticalFlowPyrLK(filter->prev_gray, gray, filter->prev_pts, curr_pts,
                                            status, err, cv::Size(OPENCV_PARAMS::WIN_SIZE_DEFAULT, OPENCV_PARAMS::WIN_SIZE_DEFAULT),
                                        filter->maxLevel,
                                        cv::TermCriteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS,
                                                       filter->maxCount, filter->epsilon),
                                        0, filter->minEigThreshold);
                
                // Filter out bad points
                std::vector<cv::Point2f> good_prev, good_curr;
                for (size_t i = 0; i < status.size(); i++) {
                    if (status[i]) {
                        good_prev.push_back(filter->prev_pts[i]);
                        good_curr.push_back(curr_pts[i]);
                    }
                }
                
                if (good_prev.size() >= OPENCV_PARAMS::MIN_FEATURES_FOR_TRANSFORM) {
                    // Calculate transform
                    cv::Mat transform = calculate_transform(good_prev, good_curr);
                    
                    // Add to transform history
                    filter->transforms.push_back(transform);
                        if (filter->transforms.size() > (size_t)smoothing_radius_copy * 2) {
                            filter->transforms.pop_front();
                        }
                        
                        // Calculate smoothed transform
                        cv::Mat smooth = smooth_transform(filter->transforms, smoothing_radius_copy);
                    
                    // Apply inverse transform to stabilize
                    cv::Mat stabilization_transform = smooth.inv();
                    
                    // Apply stabilization based on format
                    if (frame->format == VIDEO_FORMAT_BGRA) {
                        cv::Mat stabilized;
                        cv::warpPerspective(current_frame, stabilized, stabilization_transform,
                                           current_frame.size(), cv::INTER_LINEAR,
                                           cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));
                        
                        // Safe copy with overflow detection
                        size_t required_size = (size_t)frame->linesize[0] * (size_t)height;
                        if (frame->linesize[0] > 0 && height > 0 && 
                            required_size / (size_t)frame->linesize[0] == (size_t)height &&
                            required_size <= stabilized.total() * stabilized.elemSize()) {
                            memcpy(frame->data[0], stabilized.data, required_size);
                        } else {
                            obs_log(LOG_ERROR, "Frame size overflow detected: linesize=%u, height=%u", 
                                    frame->linesize[0], height);
                        }
                    }
                    
                    {
                        bool should_log = false;
                        uint32_t frame_count_val;
                        {
                            std::lock_guard<std::mutex> lock(filter->mutex);
                            should_log = filter->debug_mode && filter->frame_count % MEMORY::DEBUG_OUTPUT_INTERVAL == 0;
                            frame_count_val = filter->frame_count;
                        }
                        if (should_log) {
                            obs_log(LOG_INFO, "Stabilization applied: %zu features tracked", good_prev.size());
                        }
                    }
                }
                
                // Update for next frame
                filter->prev_pts = good_curr;
            }
            
            // Refresh features if too few
            {
                std::lock_guard<std::mutex> lock(filter->mutex);
                if (filter->prev_pts.size() < (size_t)(feature_count_copy / OPENCV_PARAMS::REFRESH_FEATURE_THRESHOLD_DIVISOR)) {
                    cv::goodFeaturesToTrack(gray, filter->prev_pts, feature_count_copy,
                                           quality_level_copy, min_distance_copy,
                                           cv::Mat(), block_size_copy, use_harris_copy, k_copy);
                    if (debug_mode_copy) {
                        obs_log(LOG_INFO, "Refreshed features: %zu detected", filter->prev_pts.size());
                    }
                }
            }
            
            {
                std::lock_guard<std::mutex> lock(filter->mutex);
                filter->prev_gray = gray.clone();
            }
        }
        
        // Update performance metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time = duration.count() / 1000.0; // Convert to ms
        
        {
            std::lock_guard<std::mutex> lock(filter->mutex);
            filter->avg_processing_time = (filter->avg_processing_time * filter->frame_count + processing_time) / (filter->frame_count + 1);
            filter->frame_count++;
            
            if (filter->debug_mode && filter->frame_count % MEMORY::DEBUG_OUTPUT_INTERVAL * 3 == 0) {
                obs_log(LOG_INFO, "Avg processing time: %.2f ms", filter->avg_processing_time);
            }
        }
        
    }  // End of try block
    
    } catch (const cv::Exception &e) {
        obs_log(LOG_ERROR, "OpenCV error in stabilizer: %s", e.what());
    } catch (const std::exception &e) {
        obs_log(LOG_ERROR, "Error in stabilizer: %s", e.what());
    }
    
    return frame;
}

// Preset configurations
static void apply_preset(obs_data_t *settings, const char *preset_name)
{
    if (strcmp(preset_name, "gaming") == 0) {
        obs_data_set_int(settings, "smoothing_radius", PRESETS::GAMING::SMOOTHING_RADIUS);
        obs_data_set_double(settings, "max_correction", PRESETS::GAMING::MAX_CORRECTION);
        obs_data_set_int(settings, "feature_count", PRESETS::GAMING::FEATURE_COUNT);
        obs_data_set_double(settings, "quality_level", PRESETS::GAMING::QUALITY_LEVEL);
        obs_data_set_double(settings, "min_distance", PRESETS::GAMING::MIN_DISTANCE);
    } else if (strcmp(preset_name, "streaming") == 0) {
        obs_data_set_int(settings, "smoothing_radius", PRESETS::STREAMING::SMOOTHING_RADIUS);
        obs_data_set_double(settings, "max_correction", PRESETS::STREAMING::MAX_CORRECTION);
        obs_data_set_int(settings, "feature_count", PRESETS::STREAMING::FEATURE_COUNT);
        obs_data_set_double(settings, "quality_level", PRESETS::STREAMING::QUALITY_LEVEL);
        obs_data_set_double(settings, "min_distance", PRESETS::STREAMING::MIN_DISTANCE);
    } else if (strcmp(preset_name, "recording") == 0) {
        obs_data_set_int(settings, "smoothing_radius", PRESETS::RECORDING::SMOOTHING_RADIUS);
        obs_data_set_double(settings, "max_correction", PRESETS::RECORDING::MAX_CORRECTION);
        obs_data_set_int(settings, "feature_count", PRESETS::RECORDING::FEATURE_COUNT);
        obs_data_set_double(settings, "quality_level", PRESETS::RECORDING::QUALITY_LEVEL);
        obs_data_set_double(settings, "min_distance", PRESETS::RECORDING::MIN_DISTANCE);
    }
}

// Preset callback function
static bool preset_changed_callback(void *priv, obs_properties_t *props, obs_property_t *property, 
                                   obs_data_t *settings)
{
    const char *preset = obs_data_get_string(settings, "preset");
    if (preset && strlen(preset) > 0) {
        apply_preset(settings, preset);
    }
    return true;
}

static void stabilizer_filter_get_defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, "preset", "streaming");
    obs_data_set_default_bool(settings, "enabled", true);
    obs_data_set_default_int(settings, "smoothing_radius", PARAM_RANGES::SMOOTHING_DEFAULT);
    obs_data_set_default_double(settings, "max_correction", PARAM_RANGES::CORRECTION_DEFAULT);
    obs_data_set_default_int(settings, "feature_count", PARAM_RANGES::FEATURES_DEFAULT);
    obs_data_set_default_double(settings, "quality_level", PARAM_RANGES::QUALITY_DEFAULT);
    obs_data_set_default_double(settings, "min_distance", PARAM_RANGES::DISTANCE_DEFAULT);
    obs_data_set_default_bool(settings, "debug_mode", false);
}

static obs_properties_t *stabilizer_filter_get_properties(void *data)
{
    UNUSED_PARAMETER(data);
    
    obs_properties_t *props = obs_properties_create();
    
    // Preset selection
    obs_property_t *preset_list = obs_properties_add_list(props, "preset", "Preset",
                                                         OBS_COMBO_TYPE_LIST, 
                                                         OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(preset_list, "Custom", "custom");
    obs_property_list_add_string(preset_list, "Gaming (Low Latency)", "gaming");
    obs_property_list_add_string(preset_list, "Streaming (Balanced)", "streaming");
    obs_property_list_add_string(preset_list, "Recording (High Quality)", "recording");
    obs_property_set_modified_callback(preset_list, preset_changed_callback);
    
    obs_properties_add_bool(props, "enabled", "Enable Stabilization");
    
    obs_properties_add_int_slider(props, "smoothing_radius", 
                                  "Smoothing Radius", 5, 100, 1);
    
    obs_properties_add_float_slider(props, "max_correction", 
                                    "Max Correction (%)", 10.0, 100.0, 1.0);
    
    obs_properties_add_int_slider(props, "feature_count", 
                                  "Feature Count", 50, 500, 10);
    
    obs_properties_add_float_slider(props, "quality_level", 
                                    "Quality Level", 0.001, 0.1, 0.001);
    
    obs_properties_add_float_slider(props, "min_distance", 
                                    "Min Distance", 10.0, 100.0, 1.0);
    
    obs_properties_add_bool(props, "debug_mode", "Debug Mode");
    
    return props;
}

// Export with C linkage
extern "C" {

// Filter info structure
static struct obs_source_info stabilizer_opencv_filter = {
    "stabilizer_opencv_filter",
    OBS_SOURCE_TYPE_FILTER,
    OBS_SOURCE_VIDEO,
    stabilizer_filter_get_name,
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
    stabilizer_filter_get_properties,
    stabilizer_filter_get_defaults,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

// Module load
MODULE_EXPORT bool obs_module_load(void)
{
    obs_log(LOG_INFO, "Loading OpenCV Stabilizer plugin");
    obs_register_source(&stabilizer_opencv_filter);
    obs_log(LOG_INFO, "OpenCV Stabilizer plugin loaded successfully");
    return true;
}

MODULE_EXPORT void obs_module_unload(void)
{
    obs_log(LOG_INFO, "Unloading OpenCV Stabilizer plugin");
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return "OpenCV-based video stabilizer filter";
}

MODULE_EXPORT const char *obs_module_name(void)
{
    return "opencv-stabilizer";
}

MODULE_EXPORT const char *obs_module_author(void)
{
    return "azumag";
}

MODULE_EXPORT const char *obs_module_version(void)
{
    return "0.1.0";
}

MODULE_EXPORT void obs_module_set_pointer(obs_module_t *module)
{
    (void)module;
}

}
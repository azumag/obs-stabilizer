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

struct stabilizer_filter {
    obs_source_t *source;
    
    // Parameters - stored from create function to avoid settings crash
    bool enabled;
    int smoothing_radius;
    float max_correction;
    int feature_count;
    float quality_level;
    float min_distance;
    int block_size;
    bool use_harris;
    float k;
    int winSize;
    int maxLevel;
    int maxCount;
    float epsilon;
    float minEigThreshold;
    
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
        filter->enabled = obs_data_get_bool(settings, "enabled");
        filter->smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
        filter->max_correction = (float)obs_data_get_double(settings, "max_correction");
        filter->feature_count = (int)obs_data_get_int(settings, "feature_count");
        filter->quality_level = (float)obs_data_get_double(settings, "quality_level");
        filter->min_distance = (float)obs_data_get_double(settings, "min_distance");
        filter->debug_mode = obs_data_get_bool(settings, "debug_mode");
        
        obs_log(LOG_INFO, "Loaded settings - enabled: %s, smoothing: %d, features: %d",
                filter->enabled ? "true" : "false",
                filter->smoothing_radius,
                filter->feature_count);
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
    // WORKAROUND: Don't access settings in update function to avoid crash
    // Settings are already read in create function
    // See docs/issue_001_settings_crash.md for details
    
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    if (!filter) {
        obs_log(LOG_WARNING, "Update called with NULL filter data");
        return;
    }
    
    obs_log(LOG_DEBUG, "Stabilizer filter update called (settings access skipped to avoid crash)");
    
    // NOTE: If we need to update settings in the future, we should:
    // 1. Use properties callbacks instead of update
    // 2. Or find a way to validate settings pointer before access
    // 3. Or investigate OBS API for proper settings handling
}

static struct obs_source_frame *stabilizer_filter_video(void *data, struct obs_source_frame *frame)
{
    struct stabilizer_filter *filter = (struct stabilizer_filter *)data;
    
    if (!filter || !frame || !filter->enabled) {
        return frame;
    }
    
    std::lock_guard<std::mutex> lock(filter->mutex);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Get frame dimensions
        uint32_t width = frame->width;
        uint32_t height = frame->height;
        
        // Check if dimensions changed
        if (filter->width != width || filter->height != height) {
            filter->width = width;
            filter->height = height;
            filter->first_frame = true;
            filter->prev_gray.release();
            filter->prev_pts.clear();
            filter->transforms.clear();
            filter->cumulative_transform = cv::Mat::eye(3, 3, CV_64F);
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
            if (filter->debug_mode) {
                obs_log(LOG_WARNING, "Unsupported video format: %d", frame->format);
            }
            return frame;
        }
        
        if (filter->first_frame) {
            // First frame - just detect features
            cv::goodFeaturesToTrack(gray, filter->prev_pts, filter->feature_count,
                                   filter->quality_level, filter->min_distance,
                                   cv::Mat(), filter->block_size, filter->use_harris, filter->k);
            filter->prev_gray = gray.clone();
            filter->first_frame = false;
            
            if (filter->debug_mode) {
                obs_log(LOG_INFO, "First frame processed, detected %zu features", filter->prev_pts.size());
            }
        } else {
            // Track features
            std::vector<cv::Point2f> curr_pts;
            std::vector<uchar> status;
            std::vector<float> err;
            
            if (!filter->prev_pts.empty()) {
                cv::calcOpticalFlowPyrLK(filter->prev_gray, gray, filter->prev_pts, curr_pts,
                                        status, err, cv::Size(filter->winSize, filter->winSize),
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
                
                if (good_prev.size() >= 4) {
                    // Calculate transform
                    cv::Mat transform = calculate_transform(good_prev, good_curr);
                    
                    // Add to transform history
                    filter->transforms.push_back(transform);
                    if (filter->transforms.size() > (size_t)filter->smoothing_radius * 2) {
                        filter->transforms.pop_front();
                    }
                    
                    // Calculate smoothed transform
                    cv::Mat smooth = smooth_transform(filter->transforms, filter->smoothing_radius);
                    
                    // Apply inverse transform to stabilize
                    cv::Mat stabilization_transform = smooth.inv();
                    
                    // Apply stabilization based on format
                    if (frame->format == VIDEO_FORMAT_BGRA) {
                        cv::Mat stabilized;
                        cv::warpPerspective(current_frame, stabilized, stabilization_transform,
                                          current_frame.size(), cv::INTER_LINEAR,
                                          cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0, 0));
                        
                        // Copy back to frame
                        memcpy(frame->data[0], stabilized.data, frame->linesize[0] * height);
                    }
                    
                    if (filter->debug_mode && filter->frame_count % 30 == 0) {
                        obs_log(LOG_INFO, "Stabilization applied: %zu features tracked", good_prev.size());
                    }
                }
                
                // Update for next frame
                filter->prev_pts = good_curr;
            }
            
            // Refresh features if too few
            if (filter->prev_pts.size() < (size_t)(filter->feature_count / 2)) {
                cv::goodFeaturesToTrack(gray, filter->prev_pts, filter->feature_count,
                                       filter->quality_level, filter->min_distance,
                                       cv::Mat(), filter->block_size, filter->use_harris, filter->k);
                if (filter->debug_mode) {
                    obs_log(LOG_INFO, "Refreshed features: %zu detected", filter->prev_pts.size());
                }
            }
            
            filter->prev_gray = gray.clone();
        }
        
        // Update performance metrics
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double processing_time = duration.count() / 1000.0; // Convert to ms
        
        filter->avg_processing_time = (filter->avg_processing_time * filter->frame_count + processing_time) / (filter->frame_count + 1);
        filter->frame_count++;
        
        if (filter->debug_mode && filter->frame_count % 100 == 0) {
            obs_log(LOG_INFO, "Avg processing time: %.2f ms", filter->avg_processing_time);
        }
        
    } catch (const cv::Exception &e) {
        obs_log(LOG_ERROR, "OpenCV error in stabilizer: %s", e.what());
    } catch (const std::exception &e) {
        obs_log(LOG_ERROR, "Error in stabilizer: %s", e.what());
    }
    
    return frame;
}

static void stabilizer_filter_get_defaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, "enabled", true);
    obs_data_set_default_int(settings, "smoothing_radius", 30);
    obs_data_set_default_double(settings, "max_correction", 50.0);
    obs_data_set_default_int(settings, "feature_count", 200);
    obs_data_set_default_double(settings, "quality_level", 0.01);
    obs_data_set_default_double(settings, "min_distance", 30.0);
    obs_data_set_default_bool(settings, "debug_mode", false);
}

static obs_properties_t *stabilizer_filter_get_properties(void *data)
{
    UNUSED_PARAMETER(data);
    
    obs_properties_t *props = obs_properties_create();
    
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
    .id = "stabilizer_opencv_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = stabilizer_filter_get_name,
    .create = stabilizer_filter_create,
    .destroy = stabilizer_filter_destroy,
    .update = stabilizer_filter_update,
    .get_defaults = stabilizer_filter_get_defaults,
    .get_properties = stabilizer_filter_get_properties,
    .filter_video = stabilizer_filter_video
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
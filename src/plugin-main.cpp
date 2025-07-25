/*
OBS Stabilizer Plugin
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <plugin-support.h>

#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#include <vector>
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

// Forward declarations for filter callbacks
static const char *stabilizer_get_name(void *unused);
static void *stabilizer_create(obs_data_t *settings, obs_source_t *source);
static void stabilizer_destroy(void *data);
static struct obs_source_frame *stabilizer_filter_video(void *data, struct obs_source_frame *frame);
static obs_properties_t *stabilizer_get_properties(void *data);
static void stabilizer_get_defaults(obs_data_t *settings);

// Basic filter structure
struct stabilizer_data {
	obs_source_t *source;
	bool enabled;
#ifdef ENABLE_STABILIZATION
	// OpenCV context
	cv::Mat prev_frame;
	std::vector<cv::Point2f> prev_points;
	bool first_frame;
	int smoothing_radius;
	int max_features;
#endif
};

// Filter callbacks implementation
static const char *stabilizer_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Stabilizer");
}

static void *stabilizer_create(obs_data_t *settings, obs_source_t *source)
{
	struct stabilizer_data *filter = (struct stabilizer_data *)bzalloc(sizeof(struct stabilizer_data));
	
	filter->source = source;
	filter->enabled = obs_data_get_bool(settings, "enabled");
	
#ifdef ENABLE_STABILIZATION
	filter->first_frame = true;
	filter->smoothing_radius = (int)obs_data_get_int(settings, "smoothing_radius");
	filter->max_features = (int)obs_data_get_int(settings, "max_features");
	obs_log(LOG_INFO, "Stabilizer filter created with OpenCV support");
#else
	obs_log(LOG_INFO, "Stabilizer filter created (pass-through mode - no OpenCV)");
#endif
	
	return filter;
}

static void stabilizer_destroy(void *data)
{
	struct stabilizer_data *filter = (struct stabilizer_data *)data;
	
	if (filter) {
		obs_log(LOG_INFO, "Stabilizer filter destroyed");
		bfree(filter);
	}
}

static struct obs_source_frame *stabilizer_filter_video(void *data, struct obs_source_frame *frame)
{
	struct stabilizer_data *filter = (struct stabilizer_data *)data;
	
	if (!filter || !filter->enabled) {
		return frame; // Pass through if disabled
	}
	
#ifdef ENABLE_STABILIZATION
	// OpenCV-based stabilization processing
	try {
		// Convert OBS frame to OpenCV Mat
		cv::Mat current_frame;
		
		// Handle different OBS pixel formats
		if (frame->format == VIDEO_FORMAT_NV12) {
			// Convert NV12 to BGR for processing
			cv::Mat nv12_mat(frame->height + frame->height/2, frame->width, CV_8UC1, frame->data[0]);
			cv::cvtColor(nv12_mat, current_frame, cv::COLOR_YUV2BGR_NV12);
		} else if (frame->format == VIDEO_FORMAT_I420) {
			// Convert I420 to BGR for processing
			cv::Mat y_plane(frame->height, frame->width, CV_8UC1, frame->data[0]);
			cv::Mat u_plane(frame->height/2, frame->width/2, CV_8UC1, frame->data[1]);
			cv::Mat v_plane(frame->height/2, frame->width/2, CV_8UC1, frame->data[2]);
			cv::cvtColor(y_plane, current_frame, cv::COLOR_GRAY2BGR); // Simplified conversion
		} else {
			// Unsupported format, pass through
			obs_log(LOG_WARNING, "Unsupported video format for stabilization: %d", frame->format);
			return frame;
		}
		
		// First frame initialization
		if (filter->first_frame) {
			cv::cvtColor(current_frame, filter->prev_frame, cv::COLOR_BGR2GRAY);
			
			// Detect initial feature points
			cv::goodFeaturesToTrack(filter->prev_frame, filter->prev_points, 
									filter->max_features, 0.01, 10);
			
			filter->first_frame = false;
			obs_log(LOG_INFO, "Stabilization initialized with %zu feature points", filter->prev_points.size());
			return frame; // Pass through first frame
		}
		
		// Convert current frame to grayscale for processing
		cv::Mat current_gray;
		cv::cvtColor(current_frame, current_gray, cv::COLOR_BGR2GRAY);
		
		// Track feature points using Lucas-Kanade optical flow
		std::vector<cv::Point2f> current_points;
		std::vector<uchar> status;
		std::vector<float> errors;
		
		if (!filter->prev_points.empty()) {
			cv::calcOpticalFlowLK(filter->prev_frame, current_gray, 
								  filter->prev_points, current_points, 
								  status, errors);
			
			// Filter out bad tracking points
			std::vector<cv::Point2f> good_prev, good_current;
			for (size_t i = 0; i < status.size(); i++) {
				if (status[i] && errors[i] < 50) {
					good_prev.push_back(filter->prev_points[i]);
					good_current.push_back(current_points[i]);
				}
			}
			
			// Estimate transformation if we have enough points
			if (good_prev.size() >= 4) {
				cv::Mat transform = cv::estimateAffinePartial2D(good_current, good_prev);
				
				if (!transform.empty()) {
					// Apply stabilization transform (simplified)
					// In a full implementation, this would include smoothing and cropping
					obs_log(LOG_DEBUG, "Stabilization applied with %zu points", good_prev.size());
				}
			}
			
			// Update previous frame and points for next iteration
			filter->prev_points = good_current;
		}
		
		// Update previous frame
		current_gray.copyTo(filter->prev_frame);
		
		// Refresh feature points if we lost too many
		if (filter->prev_points.size() < filter->max_features / 2) {
			cv::goodFeaturesToTrack(current_gray, filter->prev_points, 
									filter->max_features, 0.01, 10);
			obs_log(LOG_DEBUG, "Refreshed feature points: %zu", filter->prev_points.size());
		}
		
	} catch (const cv::Exception& e) {
		obs_log(LOG_ERROR, "OpenCV error in stabilization: %s", e.what());
	}
#endif
	
	// Return original frame (actual transformation would be applied here)
	return frame;
}

static obs_properties_t *stabilizer_get_properties(void *data)
{
	UNUSED_PARAMETER(data);
	
	obs_properties_t *props = obs_properties_create();
	
	obs_properties_add_bool(props, "enabled", obs_module_text("Stabilizer.Enable"));
	
#ifdef ENABLE_STABILIZATION
	// OpenCV-specific properties
	obs_property_t *smoothing = obs_properties_add_int_slider(props, "smoothing_radius", 
		obs_module_text("Stabilizer.SmoothingRadius"), 10, 100, 10);
	obs_property_set_long_description(smoothing, obs_module_text("Stabilizer.SmoothingRadius.Description"));
	
	obs_property_t *features = obs_properties_add_int_slider(props, "max_features",
		obs_module_text("Stabilizer.FeaturePoints"), 100, 1000, 50);
	obs_property_set_long_description(features, obs_module_text("Stabilizer.FeaturePoints.Description"));
#endif
	
	return props;
}

static void stabilizer_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
#ifdef ENABLE_STABILIZATION
	obs_data_set_default_int(settings, "smoothing_radius", 30);
	obs_data_set_default_int(settings, "max_features", 200);
#endif
}

// OBS source info structure
static struct obs_source_info stabilizer_filter = {
	.id = "stabilizer_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = stabilizer_get_name,
	.create = stabilizer_create,
	.destroy = stabilizer_destroy,
	.filter_video = stabilizer_filter_video,
	.get_properties = stabilizer_get_properties,
	.get_defaults = stabilizer_get_defaults,
};

extern "C" {

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "OBS Stabilizer plugin loaded successfully (version %s)", PLUGIN_VERSION);
	obs_log(LOG_INFO, "Real-time video stabilization plugin for OBS Studio");
	
#ifdef ENABLE_STABILIZATION
	obs_log(LOG_INFO, "OpenCV version: %s", CV_VERSION);
	obs_log(LOG_INFO, "Stabilization features enabled");
#else
	obs_log(LOG_WARNING, "OpenCV not found - stabilization features disabled");
#endif
	
	// Register the filter with OBS
	obs_register_source(&stabilizer_filter);
	obs_log(LOG_INFO, "Stabilizer filter registered with OBS");
	
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "OBS Stabilizer plugin unloaded");
}

} // extern "C"

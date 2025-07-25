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

#ifdef ENABLE_STABILIZATION
// Helper functions for frame transformation
static void apply_transform_nv12(struct obs_source_frame *frame, const cv::Mat& transform);
static void apply_transform_i420(struct obs_source_frame *frame, const cv::Mat& transform);
#endif

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
	cv::Mat working_frame;  // Pre-allocated for efficiency
	cv::Mat working_gray;   // Pre-allocated for efficiency
	std::vector<cv::Point2f> prev_points;
	cv::Mat accumulated_transform; // Accumulated transformation matrix
	cv::Mat smoothed_transform;    // Smoothed transformation matrix
	std::vector<cv::Mat> transform_history; // For smoothing
	bool first_frame;
	int smoothing_radius;
	int max_features;
#endif
};

#ifdef ENABLE_STABILIZATION
// Apply transformation to NV12 format frame
static void apply_transform_nv12(struct obs_source_frame *frame, const cv::Mat& transform)
{
	try {
		// Create OpenCV Mat for Y plane
		cv::Mat y_plane(frame->height, frame->width, CV_8UC1, frame->data[0], frame->linesize[0]);
		cv::Mat y_transformed;
		
		// Apply transformation to Y plane
		cv::warpAffine(y_plane, y_transformed, transform, 
					   cv::Size(frame->width, frame->height), 
					   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));
		
		// Copy transformed Y plane back
		y_transformed.copyTo(y_plane);
		
		// For UV plane, apply transformation at half resolution
		cv::Mat uv_plane(frame->height/2, frame->width/2, CV_8UC2,
						frame->data[0] + frame->linesize[0] * frame->height,
						frame->linesize[0]);
		cv::Mat uv_transformed;
		
		// Scale transform for half-resolution UV plane
		cv::Mat uv_transform = transform.clone();
		uv_transform.at<double>(0, 2) /= 2.0; // Scale translation X
		uv_transform.at<double>(1, 2) /= 2.0; // Scale translation Y
		
		cv::warpAffine(uv_plane, uv_transformed, uv_transform,
					   cv::Size(frame->width/2, frame->height/2),
					   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128, 128));
		
		// Copy transformed UV plane back
		uv_transformed.copyTo(uv_plane);
		
	} catch (const cv::Exception& e) {
		obs_log(LOG_ERROR, "NV12 transformation failed: %s", e.what());
	}
}

// Apply transformation to I420 format frame
static void apply_transform_i420(struct obs_source_frame *frame, const cv::Mat& transform)
{
	try {
		// Transform Y plane
		cv::Mat y_plane(frame->height, frame->width, CV_8UC1, frame->data[0], frame->linesize[0]);
		cv::Mat y_transformed;
		
		cv::warpAffine(y_plane, y_transformed, transform,
					   cv::Size(frame->width, frame->height),
					   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0));
		y_transformed.copyTo(y_plane);
		
		// Scale transform for half-resolution chroma planes  
		cv::Mat chroma_transform = transform.clone();
		chroma_transform.at<double>(0, 2) /= 2.0; // Scale translation X
		chroma_transform.at<double>(1, 2) /= 2.0; // Scale translation Y
		
		// Transform U plane
		cv::Mat u_plane(frame->height/2, frame->width/2, CV_8UC1, frame->data[1], frame->linesize[1]);
		cv::Mat u_transformed;
		
		cv::warpAffine(u_plane, u_transformed, chroma_transform,
					   cv::Size(frame->width/2, frame->height/2),
					   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128));
		u_transformed.copyTo(u_plane);
		
		// Transform V plane
		cv::Mat v_plane(frame->height/2, frame->width/2, CV_8UC1, frame->data[2], frame->linesize[2]);
		cv::Mat v_transformed;
		
		cv::warpAffine(v_plane, v_transformed, chroma_transform,
					   cv::Size(frame->width/2, frame->height/2),
					   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(128));
		v_transformed.copyTo(v_plane);
		
	} catch (const cv::Exception& e) {
		obs_log(LOG_ERROR, "I420 transformation failed: %s", e.what());
	}
}
#endif

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
	
	// Initialize transformation matrices
	filter->accumulated_transform = cv::Mat::eye(2, 3, CV_64F);
	filter->smoothed_transform = cv::Mat::eye(2, 3, CV_64F);
	
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
		// Input validation
		if (!frame || !frame->data[0] || frame->width == 0 || frame->height == 0) {
			obs_log(LOG_ERROR, "Invalid frame data for stabilization");
			return frame;
		}
		
		// Handle different OBS pixel formats with proper bounds checking
		if (frame->format == VIDEO_FORMAT_NV12) {
			// Validate NV12 format requirements
			if (!frame->data[0] || frame->linesize[0] < frame->width) {
				obs_log(LOG_ERROR, "Invalid NV12 frame data or linesize");
				return frame;
			}
			
			// Create Mat with proper stride handling
			cv::Mat nv12_y(frame->height, frame->width, CV_8UC1, 
						   frame->data[0], frame->linesize[0]);
			cv::Mat nv12_uv(frame->height/2, frame->width/2, CV_8UC2,
							frame->data[0] + frame->linesize[0] * frame->height,
							frame->linesize[0]);
			
			// Convert directly to grayscale for efficiency
			nv12_y.copyTo(filter->working_gray);
			
		} else if (frame->format == VIDEO_FORMAT_I420) {
			// Validate I420 format requirements  
			if (!frame->data[0] || !frame->data[1] || !frame->data[2] ||
				frame->linesize[0] < frame->width ||
				frame->linesize[1] < frame->width/2 ||
				frame->linesize[2] < frame->width/2) {
				obs_log(LOG_ERROR, "Invalid I420 frame data or linesize");
				return frame;
			}
			
			// Use Y plane directly for grayscale processing (more efficient)
			cv::Mat y_plane(frame->height, frame->width, CV_8UC1, 
						   frame->data[0], frame->linesize[0]);
			y_plane.copyTo(filter->working_gray);
			
		} else {
			// Unsupported format, pass through
			obs_log(LOG_WARNING, "Unsupported video format for stabilization: %d", frame->format);
			return frame;
		}
		
		// First frame initialization
		if (filter->first_frame) {
			filter->working_gray.copyTo(filter->prev_frame);
			
			// Validate frame dimensions for feature detection
			if (filter->working_gray.rows < 50 || filter->working_gray.cols < 50) {
				obs_log(LOG_WARNING, "Frame too small for reliable feature detection: %dx%d", 
						filter->working_gray.cols, filter->working_gray.rows);
				return frame;
			}
			
			// Detect initial feature points with bounds checking
			try {
				cv::goodFeaturesToTrack(filter->prev_frame, filter->prev_points, 
										std::max(50, std::min(filter->max_features, 1000)), 0.01, 10);
			} catch (const cv::Exception& e) {
				obs_log(LOG_ERROR, "Feature detection failed: %s", e.what());
				return frame;
			}
			
			filter->first_frame = false;
			obs_log(LOG_INFO, "Stabilization initialized with %zu feature points", filter->prev_points.size());
			return frame; // Pass through first frame
		}
		
		// Track feature points using Lucas-Kanade optical flow
		std::vector<cv::Point2f> current_points;
		std::vector<uchar> status;
		std::vector<float> errors;
		
		if (!filter->prev_points.empty()) {
			try {
				cv::calcOpticalFlowLK(filter->prev_frame, filter->working_gray, 
									  filter->prev_points, current_points, 
									  status, errors);
			} catch (const cv::Exception& e) {
				obs_log(LOG_ERROR, "Optical flow calculation failed: %s", e.what());
				// Reset state for recovery
				filter->first_frame = true;
				filter->prev_points.clear();
				return frame;
			}
			
			// Filter out bad tracking points with improved thresholds
			std::vector<cv::Point2f> good_prev, good_current;
			const float max_error = 30.0f; // Reduced from 50 for better quality
			
			for (size_t i = 0; i < status.size() && i < errors.size(); i++) {
				if (status[i] && errors[i] < max_error) {
					// Additional bounds checking for points
					if (current_points[i].x >= 0 && current_points[i].x < filter->working_gray.cols &&
						current_points[i].y >= 0 && current_points[i].y < filter->working_gray.rows) {
						good_prev.push_back(filter->prev_points[i]);
						good_current.push_back(current_points[i]);
					}
				}
			}
			
			// Estimate transformation if we have enough points
			const size_t min_points = 6; // Increased from 4 for stability
			if (good_prev.size() >= min_points) {
				try {
					cv::Mat transform = cv::estimateAffinePartial2D(good_current, good_prev, 
																	cv::noArray(), cv::RANSAC, 3.0);
					
					if (!transform.empty()) {
						// Accumulate transformation
						filter->accumulated_transform = transform * filter->accumulated_transform;
						
						// Add to history for smoothing
						filter->transform_history.push_back(transform.clone());
						
						// Keep only recent transforms for smoothing
						size_t max_history = static_cast<size_t>(filter->smoothing_radius);
						if (filter->transform_history.size() > max_history) {
							filter->transform_history.erase(filter->transform_history.begin());
						}
						
						// Calculate smoothed transform (moving average)
						cv::Mat smoothed = cv::Mat::zeros(2, 3, CV_64F);
						for (const auto& hist_transform : filter->transform_history) {
							smoothed += hist_transform;
						}
						smoothed /= static_cast<double>(filter->transform_history.size());
						filter->smoothed_transform = smoothed;
						
						// Apply stabilization transform to the frame
						if (frame->format == VIDEO_FORMAT_NV12) {
							apply_transform_nv12(frame, filter->smoothed_transform);
						} else if (frame->format == VIDEO_FORMAT_I420) {
							apply_transform_i420(frame, filter->smoothed_transform);
						}
						
						obs_log(LOG_DEBUG, "Transform applied with %zu points", good_prev.size());
					}
				} catch (const cv::Exception& e) {
					obs_log(LOG_WARNING, "Transform estimation failed: %s", e.what());
				}
			} else {
				obs_log(LOG_DEBUG, "Insufficient tracking points: %zu (need %zu)", good_prev.size(), min_points);
			}
			
			// Update previous points with validated good points
			filter->prev_points = good_current;
		}
		
		// Update previous frame
		filter->working_gray.copyTo(filter->prev_frame);
		
		// Refresh feature points if we lost too many (with bounds checking)
		const size_t refresh_threshold = std::max(static_cast<size_t>(25), 
												  static_cast<size_t>(filter->max_features / 3));
		if (filter->prev_points.size() < refresh_threshold) {
			try {
				std::vector<cv::Point2f> new_points;
				cv::goodFeaturesToTrack(filter->working_gray, new_points, 
										std::max(50, std::min(filter->max_features, 1000)), 0.01, 10);
				
				// Merge with existing points if any remain
				filter->prev_points.insert(filter->prev_points.end(), new_points.begin(), new_points.end());
				obs_log(LOG_DEBUG, "Refreshed feature points: total %zu", filter->prev_points.size());
			} catch (const cv::Exception& e) {
				obs_log(LOG_ERROR, "Feature refresh failed: %s", e.what());
			}
		}
		
	} catch (const cv::Exception& e) {
		obs_log(LOG_ERROR, "OpenCV error in stabilization: %s", e.what());
		// Reset filter state for recovery
		filter->first_frame = true;
		filter->prev_points.clear();
	} catch (const std::exception& e) {
		obs_log(LOG_ERROR, "Standard exception in stabilization: %s", e.what());
		filter->first_frame = true;
		filter->prev_points.clear();
	} catch (...) {
		obs_log(LOG_ERROR, "Unknown exception in stabilization");
		filter->first_frame = true;
		filter->prev_points.clear();
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

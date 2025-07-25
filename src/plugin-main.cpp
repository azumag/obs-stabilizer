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
	// Future: Add OpenCV processing context here
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
	
	obs_log(LOG_INFO, "Stabilizer filter created");
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
	
	// TODO: Implement actual stabilization here
	// For now, just pass through the frame
	return frame;
}

static obs_properties_t *stabilizer_get_properties(void *data)
{
	UNUSED_PARAMETER(data);
	
	obs_properties_t *props = obs_properties_create();
	
	obs_properties_add_bool(props, "enabled", obs_module_text("Stabilizer.Enable"));
	
	// TODO: Add more properties (smoothing radius, feature points, etc.)
	
	return props;
}

static void stabilizer_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enabled", true);
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

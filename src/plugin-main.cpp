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
	
	return true;
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "OBS Stabilizer plugin unloaded");
}

} // extern "C"

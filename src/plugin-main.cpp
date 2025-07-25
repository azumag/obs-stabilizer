/*
OBS Stabilizer Plugin - Main Plugin Entry Point (Refactored)
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include <obs-module.h>
#include <plugin-support.h>
#include "obs/obs_integration.hpp"

// OpenCV API compatibility validation
#ifdef ENABLE_STABILIZATION
#include <opencv2/opencv.hpp>

#if !defined(CV_VERSION_MAJOR) || (CV_VERSION_MAJOR < 4) || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR < 5)
    #error "OpenCV 4.5+ is required for this plugin"
#endif

// Version-specific compatibility checks
#if CV_VERSION_MAJOR >= 5
    #warning "OpenCV 5.x detected - some APIs may have changed. Verify functionality."
#endif
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Real-time video stabilization plugin for OBS Studio using OpenCV";
}

MODULE_EXPORT bool obs_module_load(void)
{
    obs_log(LOG_INFO, "Loading OBS Stabilizer Plugin v%s", PLUGIN_VERSION);
    
#ifdef ENABLE_STABILIZATION
    obs_log(LOG_INFO, "OpenCV version: %s", CV_VERSION);
    obs_log(LOG_INFO, "Stabilization features: ENABLED");
#else
    obs_log(LOG_WARNING, "Stabilization features: DISABLED (OpenCV not found)");
#endif
    
    // Initialize the OBS integration layer
    return obs_stabilizer::OBSIntegration::plugin_load();
}

MODULE_EXPORT void obs_module_unload(void)
{
    obs_log(LOG_INFO, "Unloading OBS Stabilizer Plugin");
    
    // Clean up the OBS integration layer
    obs_stabilizer::OBSIntegration::plugin_unload();
}
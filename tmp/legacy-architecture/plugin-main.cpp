/*
OBS Stabilizer Plugin - Main Plugin Entry Point (Refactored)
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>
#include "obs/obs_integration.hpp"
#endif

#include <plugin-support.h>

#ifndef HAVE_OBS_HEADERS
#include <stdio.h>
#endif

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

#ifdef HAVE_OBS_HEADERS
// OBS module functions are now defined in obs_module_exports.c

/**
 * @brief C++ implementation of plugin load functionality
 * Called by the C export layer to initialize the plugin
 * @return true if plugin loaded successfully, false otherwise
 */
extern "C" bool obs_stabilizer_plugin_load(void)
{
    try {
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
    catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Plugin load failed with exception: %s", e.what());
        return false;
    }
    catch (...) {
        obs_log(LOG_ERROR, "Plugin load failed with unknown exception");
        return false;
    }
}


/**
 * @brief C++ implementation of plugin unload functionality
 * Called by the C export layer to cleanup the plugin
 */
extern "C" void obs_stabilizer_plugin_unload(void)
{
    try {
        obs_log(LOG_INFO, "Unloading OBS Stabilizer Plugin");

        // Clean up the OBS integration layer
        obs_stabilizer::OBSIntegration::plugin_unload();
    }
    catch (const std::exception& e) {
        obs_log(LOG_ERROR, "Plugin unload failed with exception: %s", e.what());
    }
    catch (...) {
        obs_log(LOG_ERROR, "Plugin unload failed with unknown exception");
    }
}

#else // BUILD_STANDALONE_TEST mode

// Standalone test mode entry point for development without OBS
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    printf("OBS Stabilizer Plugin v%s - Standalone Test Mode\n", PLUGIN_VERSION);

#ifdef ENABLE_STABILIZATION
    printf("OpenCV version: %s\n", CV_VERSION);
    printf("Stabilization features: ENABLED\n");
#else
    printf("Stabilization features: DISABLED (OpenCV not found)\n");
#endif

    printf("Standalone test mode - for development without OBS installation\n");
    printf("To build as OBS plugin, install OBS headers and rebuild\n");

    return 0;
}

#endif
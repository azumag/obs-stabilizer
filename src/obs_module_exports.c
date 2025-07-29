/*
OBS Stabilizer Plugin - Module Export Layer
Copyright (C) 2025 azumag <azumag@users.noreply.github.com>

Pure C implementation to ensure proper C linkage for OBS module functions.
This layer prevents C++ name mangling issues that prevent OBS from loading the plugin.
*/

#ifdef HAVE_OBS_HEADERS
#include <obs-module.h>
#endif

#include "plugin-support.h"
#include <stdbool.h>

#ifndef HAVE_OBS_HEADERS
#error "This file requires OBS headers to be available"
#endif

// Forward declarations for C++ wrapper functions
extern bool obs_stabilizer_plugin_load(void);
extern void obs_stabilizer_plugin_unload(void);

/**
 * @brief Returns the plugin name for OBS module registration
 * @return Plugin name string constant
 */
MODULE_EXPORT const char *obs_module_name(void)
{
    return PLUGIN_NAME;
}

/**
 * @brief Returns the plugin description for OBS module registration
 * @return Plugin description string
 */
MODULE_EXPORT const char *obs_module_description(void)
{
    return "Real-time video stabilization plugin for OBS Studio using OpenCV";
}

/**
 * @brief OBS module load entry point - delegates to C++ implementation
 * @return true if plugin loaded successfully, false otherwise
 */
MODULE_EXPORT bool obs_module_load(void)
{
    return obs_stabilizer_plugin_load();
}

/**
 * @brief OBS module text localization entry point - delegates to stub for now
 * @param lookup_string The text key to look up  
 * @return Localized text string
 */
#ifndef USE_OBS_STUBS
MODULE_EXPORT const char* obs_module_text(const char* lookup_string)
{
    // For plugin builds, just return the string (no localization for now)
    return lookup_string ? lookup_string : "";
}
#endif

/**
 * @brief OBS module unload entry point - delegates to C++ implementation
 */
MODULE_EXPORT void obs_module_unload(void)
{
    obs_stabilizer_plugin_unload();
}
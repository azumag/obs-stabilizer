/*
OBS Stabilizer Plugin - Video Filter Implementation
Real-time video stabilization using OpenCV
*/

#include <obs-module.h>
#include <stdio.h>

// Remove forward declaration - we'll handle registration in stabilizer_filter.cpp

// Module metadata
#define PLUGIN_VERSION "0.1.0"

OBS_DECLARE_MODULE()

// Ensure C linkage for OBS module functions
#ifdef __cplusplus
extern "C" {
#endif

MODULE_EXPORT const char* obs_module_description(void) {
    return "Real-time video stabilization plugin for OBS Studio";
}

MODULE_EXPORT const char* obs_module_author(void) {
    return "azumag";
}

// This will be defined in simple_passthrough.cpp
extern bool simple_obs_module_load(void);

MODULE_EXPORT bool obs_module_load(void) {
    // Call the simple module load function
    return simple_obs_module_load();
}

MODULE_EXPORT void obs_module_unload(void) {
    obs_log(LOG_INFO, "[obs-stabilizer] Unloading OBS Stabilizer Plugin");
}

MODULE_EXPORT const char* obs_module_name(void) {
    return "test-stabilizer";
}

// Critical: OBS version compatibility - this symbol is required for plugin discovery
MODULE_EXPORT uint32_t obs_module_ver(void) {  
    return 0x1c000000;  // OBS 28.0.0 compatible version (matching audio-monitor)
}

#ifdef __cplusplus
}
#endif
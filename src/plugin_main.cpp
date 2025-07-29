/*
OBS Stabilizer Plugin - Simplified Entry Point
Architectural simplification following Gemini review requirements
*/

#include <obs-module.h>
// Forward declaration
void register_stabilizer_filter();

// Module metadata
#define PLUGIN_VERSION "0.1.0"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-stabilizer", "en-US")

MODULE_EXPORT const char* obs_module_description(void) {
    return "Real-time video stabilization plugin for OBS Studio";
}

MODULE_EXPORT const char* obs_module_author(void) {
    return "azumag";
}

bool obs_module_load(void) {
    obs_log(LOG_INFO, "Loading OBS Stabilizer Plugin v%s", PLUGIN_VERSION);
    
    // Register the stabilizer filter
    register_stabilizer_filter();
    
    obs_log(LOG_INFO, "OBS Stabilizer Plugin loaded successfully");
    return true;
}

void obs_module_unload(void) {
    obs_log(LOG_INFO, "Unloading OBS Stabilizer Plugin");
}
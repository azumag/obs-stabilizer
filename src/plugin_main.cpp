/*
OBS Stabilizer Plugin - Video Filter Implementation
Real-time video stabilization using OpenCV
*/

#include <obs-module.h>
#include <stdio.h>

// Forward declaration for filter registration
#ifdef __cplusplus
extern "C" {
#endif
extern void register_stabilizer_filter(void);
#ifdef __cplusplus
}
#endif

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

MODULE_EXPORT bool obs_module_load(void) {
    // Register the video stabilizer filter
    register_stabilizer_filter();
    printf("[obs-stabilizer] Video stabilizer filter registered\n");
    fflush(stdout);
    return true;
}

MODULE_EXPORT void obs_module_unload(void) {
    printf("[obs-stabilizer] Unloading OBS Stabilizer Plugin\n");
    fflush(stdout);
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
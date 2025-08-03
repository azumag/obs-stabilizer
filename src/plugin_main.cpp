/*
OBS Stabilizer Plugin - Simplified Entry Point
Architectural simplification following Gemini review requirements
*/

#include <obs-module.h>
#include <stdio.h>
// Forward declaration
void register_stabilizer_filter();

// Module metadata
#define PLUGIN_VERSION "0.1.0"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("test-stabilizer", "en-US")

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
    printf("[obs-stabilizer] Loading OBS Stabilizer Plugin v%s\n", PLUGIN_VERSION);
    fflush(stdout);
    
    try {
        // Register the stabilizer filter
        register_stabilizer_filter();
        printf("[obs-stabilizer] Stabilizer filter registered successfully\n");
        fflush(stdout);
    } catch (...) {
        printf("[obs-stabilizer] Failed to register stabilizer filter\n");
        fflush(stdout);
        return false;
    }
    
    printf("[obs-stabilizer] OBS Stabilizer Plugin loaded successfully\n");
    fflush(stdout);
    return true;
}

MODULE_EXPORT void obs_module_unload(void) {
    printf("[obs-stabilizer] Unloading OBS Stabilizer Plugin\n");
    fflush(stdout);
}

// Required locale functions for OBS_MODULE_USE_DEFAULT_LOCALE
MODULE_EXPORT void obs_module_set_locale(const char *locale) {
    // This function is required by OBS_MODULE_USE_DEFAULT_LOCALE macro
    (void)locale; // Suppress unused parameter warning
}

MODULE_EXPORT void obs_module_free_locale(void) {
    // This function is required by OBS_MODULE_USE_DEFAULT_LOCALE macro
}

MODULE_EXPORT const char *obs_module_get_string(const char *lookup_string) {
    // This function is required by OBS_MODULE_USE_DEFAULT_LOCALE macro
    // For now, just return the lookup string itself
    return lookup_string;
}

// Critical: OBS version compatibility - this symbol is required for plugin discovery
MODULE_EXPORT uint32_t obs_module_ver(void) {
    return 0x1f010002;  // Matches mac-capture plugin version
}

#ifdef __cplusplus
}
#endif
/*
Minimal OBS Plugin Test - No External Dependencies
Diagnostic test to isolate plugin loading issues
*/

#include <obs-module.h>
#include <stdio.h>

// Module metadata
#define PLUGIN_VERSION "0.1.0-minimal"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-stabilizer-minimal", "en-US")

// Ensure C linkage for OBS module functions
#ifdef __cplusplus
extern "C" {
#endif

MODULE_EXPORT const char* obs_module_description(void) {
    return "Minimal test plugin for OBS Studio - no external dependencies";
}

MODULE_EXPORT const char* obs_module_author(void) {
    return "azumag";
}

// obs_module_name is provided by OBS_MODULE_USE_DEFAULT_LOCALE macro

MODULE_EXPORT bool obs_module_load(void) {
    printf("[obs-stabilizer-minimal] Module loading started\n");
    fflush(stdout);
    printf("[obs-stabilizer-minimal] Module loaded successfully\n");
    fflush(stdout);
    return true;
}

MODULE_EXPORT void obs_module_unload(void) {
    printf("[obs-stabilizer-minimal] Module unloaded\n");
    fflush(stdout);
}

// Required locale functions for OBS_MODULE_USE_DEFAULT_LOCALE
MODULE_EXPORT void obs_module_set_locale(const char *locale) {
    (void)locale; // Suppress unused parameter warning
}

MODULE_EXPORT void obs_module_free_locale(void) {
    // Empty implementation
}

MODULE_EXPORT const char *obs_module_get_string(const char *lookup_string) {
    return lookup_string;
}

// Critical: OBS version compatibility - this symbol is required for plugin discovery
MODULE_EXPORT uint32_t obs_module_ver(void) {
    return 0x1f010002;  // Matches mac-capture plugin version
}

#ifdef __cplusplus
}
#endif
/*
Minimal OBS Plugin Test - Debug version
Tests basic OBS logging functionality
*/

#include <obs-module.h>

extern "C" {

MODULE_EXPORT bool obs_module_load(void)
{
    // Try different logging methods to see what works
    obs_log(LOG_INFO, "MINIMAL TEST: Plugin loading with obs_log");
    
    // Try using the bridge function
    extern void blogva(int log_level, const char *format, ...);
    blogva(LOG_INFO, "MINIMAL TEST: Plugin loading with blogva");

    // Register a simple source to verify registration works
    return true;
}

MODULE_EXPORT void obs_module_unload(void)
{
    obs_log(LOG_INFO, "MINIMAL TEST: Plugin unloading");
}

MODULE_EXPORT void obs_module_set_pointer(obs_module_t *module)
{
    // Set module pointer if needed
    (void)module;
}

}
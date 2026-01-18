/*
OBS Minimal Stabilizer Plugin - Stable version
*/

#include <cstdint>
#include <cstring>
#include <cstdlib>

// OBS module API
extern "C" {

// Module pointer
void *obs_module_pointer = nullptr;

// Required OBS functions
void obs_module_set_pointer(void *module)
{
    obs_module_pointer = module;
}

void *obs_current_module(void)
{
    return obs_module_pointer;
}

bool obs_module_load(void)
{
    // Just return true - no functionality yet
    return true;
}

void obs_module_unload(void)
{
    // Nothing to unload
}

const char *obs_module_name(void)
{
    return "OBS Stabilizer Minimal";
}

const char *obs_module_description(void)
{
    return "Minimal video stabilizer plugin";
}

} // extern "C"
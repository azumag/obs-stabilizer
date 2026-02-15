/******************************************************************************
    Minimal Profiler Definitions for OBS Plugin Development
    This file provides profiler stubs for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Profiler stubs - disabled in standalone mode */
#define profile_start(name)
#define profile_end(name)
#define profile_register(name, desc)
#define profile_reenable_register()
#define profile_disable_register()
#define profile_print()

#ifdef __cplusplus
}
#endif

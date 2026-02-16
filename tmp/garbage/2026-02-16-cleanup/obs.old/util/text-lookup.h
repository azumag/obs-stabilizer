/******************************************************************************
    Minimal Text Lookup Definitions for OBS Plugin Development
    This file provides text lookup stubs for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Text lookup stubs - disabled in standalone mode */
typedef void* text_lookup_t;

#define obs_module_text(lookup_name) (lookup_name)
#define obs_module_get_string(name) (name)

#ifdef __cplusplus
}
#endif

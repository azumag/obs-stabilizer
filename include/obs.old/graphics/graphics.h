/******************************************************************************
    Minimal Graphics Definitions for OBS Plugin Development
    This file provides graphics stubs for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/* Graphics types (stubs) */
typedef void* gs_device_t;
typedef void* gs_texture_t;
typedef void* gs_shader_t;
typedef void* gs_sampler_state_t;
typedef void* gs_vertbuffer_t;
typedef void* gs_indexbuffer_t;
typedef void* gs_effect_t;
typedef void* gs_effect_t;
typedef void* gs_texrender_t;
typedef void* gs_stagesurf_t;

#ifdef __cplusplus
}
#endif

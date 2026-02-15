/******************************************************************************
    Minimal Video I/O Definitions for OBS Plugin Development
    This file provides video I/O structures for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Video format types */
typedef enum video_format {
    VIDEO_FORMAT_NONE,
    VIDEO_FORMAT_I420,
    VIDEO_FORMAT_NV12,
    VIDEO_FORMAT_YVYU,
    VIDEO_FORMAT_YUY2,
    VIDEO_FORMAT_UYVY,
    VIDEO_FORMAT_RGBA,
    VIDEO_FORMAT_BGRA,
    VIDEO_FORMAT_BGRX,
    VIDEO_FORMAT_Y800,
    VIDEO_FORMAT_I40A,
    VIDEO_FORMAT_I42A,
    VIDEO_FORMAT_I422,
    VIDEO_FORMAT_I444,
} video_format;

/* Color range types */
typedef enum video_range_type {
    VIDEO_RANGE_DEFAULT,
    VIDEO_RANGE_PARTIAL,
    VIDEO_RANGE_FULL,
} video_range_type;

#ifdef __cplusplus
}
#endif

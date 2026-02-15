/******************************************************************************
    Minimal Frame Rate Definitions for OBS Plugin Development
    This file provides frame rate structures for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../util/c99defs.h"
#include <stdint.h>

typedef struct media_frames_per_second {
    uint32_t numerator;
    uint32_t denominator;
} media_frames_per_second;

static inline bool media_frames_per_second_is_valid(
        const struct media_frames_per_second *fps)
{
    return (fps != NULL && fps->numerator != 0 && fps->denominator != 0);
}

static inline double media_frames_per_second_to_fps(
        const struct media_frames_per_second *fps)
{
    if (!media_frames_per_second_is_valid(fps))
        return 0.0;
    return (double)fps->numerator / (double)fps->denominator;
}

#ifdef __cplusplus
}
#endif

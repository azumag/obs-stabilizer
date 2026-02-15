/******************************************************************************
    Minimal Audio I/O Definitions for OBS Plugin Development
    This file provides audio I/O stubs for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Audio format types (stubs) */
typedef enum audio_format {
    AUDIO_FORMAT_UNKNOWN,
    AUDIO_FORMAT_U8BIT,
    AUDIO_FORMAT_16BIT,
    AUDIO_FORMAT_32BIT,
    AUDIO_FORMAT_FLOAT,
    AUDIO_FORMAT_U8BIT_PLANAR,
    AUDIO_FORMAT_16BIT_PLANAR,
    AUDIO_FORMAT_32BIT_PLANAR,
    AUDIO_FORMAT_FLOAT_PLANAR,
} audio_format;

/* Audio layout types (stubs) */
typedef enum speaker_layout {
    SPEAKERS_UNKNOWN,
    SPEAKERS_MONO,
    SPEAKERS_STEREO,
    SPEAKERS_2POINT1,
    SPEAKERS_4POINT0,
    SPEAKERS_4POINT1,
    SPEAKERS_5POINT1,
    SPEAKERS_5POINT1_SURROUND,
    SPEAKERS_7POINT1,
    SPEAKERS_7POINT1_SURROUND,
} speaker_layout;

#ifdef __cplusplus
}
#endif

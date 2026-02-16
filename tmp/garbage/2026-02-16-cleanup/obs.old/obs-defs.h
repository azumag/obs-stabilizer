/******************************************************************************
    Copyright (C) 2022 by Hugh Bailey <jim@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODULE_MISSING_EXPORTS -3

/* OBS Constants */
#define MAX_AUDIO_MIXES 6
#define MAX_AUDIO_CHANNELS 8
#define MAX_AV_PLANES 8
#define MAX_VIDEO_SURFACES 4

/* OBS Source Types */
typedef enum obs_source_type {
    OBS_SOURCE_TYPE_INPUT,
    OBS_SOURCE_TYPE_FILTER,
    OBS_SOURCE_TYPE_TRANSITION,
    OBS_SOURCE_TYPE_SCENE,
    OBS_SOURCE_TYPE_SCENEITEM,
} obs_source_type;

/* OBS Output Flags */
#define OBS_SOURCE_VIDEO (1 << 0)
#define OBS_SOURCE_AUDIO (1 << 1)
#define OBS_SOURCE_ASYNC (1 << 2)
#define OBS_SOURCE_CUSTOM_DRAW (1 << 3)

#ifdef __cplusplus
}
#endif
#define MODULE_INCOMPATIBLE_VER -4
#define MODULE_HARDCODED_SKIP -5

#define OBS_OUTPUT_SUCCESS 0
#define OBS_OUTPUT_BAD_PATH -1
#define OBS_OUTPUT_CONNECT_FAILED -2
#define OBS_OUTPUT_INVALID_STREAM -3
#define OBS_OUTPUT_ERROR -4
#define OBS_OUTPUT_DISCONNECTED -5
#define OBS_OUTPUT_UNSUPPORTED -6
#define OBS_OUTPUT_NO_SPACE -7
#define OBS_OUTPUT_ENCODE_ERROR -8
#define OBS_OUTPUT_HDR_DISABLED -9

#define OBS_VIDEO_SUCCESS 0
#define OBS_VIDEO_FAIL -1
#define OBS_VIDEO_NOT_SUPPORTED -2
#define OBS_VIDEO_INVALID_PARAM -3
#define OBS_VIDEO_CURRENTLY_ACTIVE -4
#define OBS_VIDEO_MODULE_NOT_FOUND -5
